/*******************************************************************************
 * cobs/document_list.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_DOCUMENT_LIST_HEADER
#define COBS_DOCUMENT_LIST_HEADER

#include <cobs/cortex_file.hpp>
#include <cobs/fasta_file.hpp>
#include <cobs/fasta_multifile.hpp>
#include <cobs/fastq_file.hpp>
#include <cobs/settings.hpp>
#include <cobs/text_file.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/parallel_for.hpp>

#include <algorithm>
#include <fstream>
#include <iomanip>

#include <tlx/logger.hpp>
#include <tlx/string/ends_with.hpp>

namespace cobs {

/******************************************************************************/

enum class FileType {
    //! Accept any supported file types in dir scan
    Any,
    //! Text file: read as sequential symbol stream
    Text,
    //! McCortex file
    Cortex,
    //! Explicit list of k-mers for testing
    KMerBuffer,
    //! FastA file, parsed as one document containing subsequences
    Fasta,
    //! FastQ file, parsed as one document containing subsequences
    Fastq,
    //! FastA multi-file, parsed as multiple documents
    FastaMulti,
    //! FastQ multi-file, parsed as multiple documents
    FastqMulti,
    //! Special filelist file type (cannot actually be a document)
    List,
};

FileType StringToFileType(std::string& s);

/******************************************************************************/

/*!
 * DocumentEntry specifies a document or subdocument which can deliver a set of
 * q-grams for indexing.
 */
struct DocumentEntry {
    //! file system path to document
    std::string path_;
    //! type of document
    FileType type_;
    //! name of the document
    std::string name_;
    //! size of the document in bytes
    size_t size_;
    //! subdocument index (for Multifile FASTA, FASTQ, etc)
    size_t subdoc_index_ = 0;
    //! fixed term (term) size or zero
    size_t term_size_ = 0;
    //! number of terms if fixed size
    size_t term_count_ = 0;

    //! default sort operator
    bool operator < (const DocumentEntry& b) const {
        return (std::tie(path_, subdoc_index_) <
                std::tie(b.path_, b.subdoc_index_));
    }

    //! calculate number of terms in file
    size_t num_terms(size_t k) const {
        if (type_ == FileType::Text) {
            return size_ < k ? 0 : size_ - k + 1;
        }
        else if (type_ == FileType::Cortex) {
            return term_size_ >= k ? term_count_ * (term_size_ - k + 1) : 0;
        }
        else if (type_ == FileType::KMerBuffer) {
            return term_size_ >= k ? term_count_ * (term_size_ - k + 1) : 0;
        }
        else if (type_ == FileType::Fasta) {
            // read fasta index
            FastaFile fasta(path_);
            return fasta.num_terms(k);
        }
        else if (type_ == FileType::FastaMulti) {
            return size_ < k ? 0 : size_ - k + 1;
        }
        else if (type_ == FileType::Fastq) {
            // read fasta index
            FastqFile fastq(path_);
            return fastq.num_terms(k);
        }
        else {
            die("DocumentEntry: unknown file type");
        }
    }

    //! process terms
    template <typename Callback>
    void process_terms(unsigned term_size, Callback callback) const {
        if (type_ == FileType::Text) {
            TextFile text(path_);
            text.process_terms(term_size, callback);
        }
        else if (type_ == FileType::Cortex) {
            CortexFile ctx(path_);
            ctx.process_terms(term_size, callback);
        }
        else if (type_ == FileType::KMerBuffer) {
            die_unequal(term_size, 31u);
            KMerBuffer<31> doc;
            KMerBufferHeader dh;
            doc.deserialize(path_, dh);

            std::string term;
            term.reserve(32);

            for (uint64_t j = 0; j < doc.data().size(); j++) {
                doc.data()[j].to_string(&term);
                callback(tlx::string_view(term));
            }
        }
        else if (type_ == FileType::Fasta) {
            FastaFile fasta(path_);
            fasta.process_terms(term_size, callback);
        }
        else if (type_ == FileType::FastaMulti) {
            FastaMultifile mfasta(path_);
            mfasta.process_terms(subdoc_index_, term_size, callback);
        }
        else if (type_ == FileType::Fastq) {
            FastqFile fastq(path_);
            fastq.process_terms(term_size, callback);
        }
        else {
            die("DocumentEntry: unknown file type");
        }
    }
};

/*!
 * DocumentList is used to scan a directory, filter the files for specific
 * document types, and then run batch processing methods on them.
 */
class DocumentList
{
public:
    using DocumentEntryList = std::vector<DocumentEntry>;

    //! empty document list
    DocumentList() = default;

    //! accept a file list, sort by name
    explicit DocumentList(const DocumentEntryList& list)
        : list_(list) {
        std::sort(list_.begin(), list_.end());
    }

    //! read a directory, filter files
    explicit DocumentList(const fs::path& root,
                          FileType filter = FileType::Any) {
        add_recursive(root, filter);
    }

    //! filter method to match file specifications
    static bool accept(const fs::path& path, FileType filter) {
        FileType ft = identify_filetype(path);
        switch (filter) {
        case FileType::Any:
            return (ft == FileType::Text) ||
                   (ft == FileType::Cortex) ||
                   (ft == FileType::KMerBuffer) ||
                   (ft == FileType::Fasta) ||
                   (ft == FileType::Fastq) ||
                   (ft == FileType::FastaMulti) ||
                   (ft == FileType::FastqMulti);
        case FileType::Text:
            return (ft == FileType::Text);
        case FileType::Cortex:
            return (ft == FileType::Cortex);
        case FileType::KMerBuffer:
            return (ft == FileType::KMerBuffer);
        case FileType::Fasta:
            return (ft == FileType::Fasta);
        case FileType::Fastq:
            return (ft == FileType::Fastq);
        case FileType::FastaMulti:
            return (ft == FileType::FastaMulti);
        case FileType::FastqMulti:
            return (ft == FileType::FastqMulti);
        case FileType::List:
            return (ft == FileType::List);
        }
        return false;
    }

    //! return FileType of path or FileType::Any for others.
    static FileType identify_filetype(const fs::path& path) {
        std::string spath = path.string();
        if (tlx::ends_with(spath, ".txt")) {
            return FileType::Text;
        }
        else if (tlx::ends_with(spath, ".ctx") ||
                 tlx::ends_with(spath, ".cortex")) {
            return FileType::Cortex;
        }
        else if (tlx::ends_with(spath, ".cobs_doc")) {
            return FileType::KMerBuffer;
        }
        else if (tlx::ends_with(spath, ".fa") ||
                 tlx::ends_with(spath, ".fa.gz") ||
                 tlx::ends_with(spath, ".fasta") ||
                 tlx::ends_with(spath, ".fasta.gz") ||
                 tlx::ends_with(spath, ".fna") ||
                 tlx::ends_with(spath, ".fna.gz") ||
                 tlx::ends_with(spath, ".ffn") ||
                 tlx::ends_with(spath, ".ffn.gz") ||
                 tlx::ends_with(spath, ".faa") ||
                 tlx::ends_with(spath, ".faa.gz") ||
                 tlx::ends_with(spath, ".frn") ||
                 tlx::ends_with(spath, ".frn.gz")){
            return FileType::Fasta;
        }
        else if (tlx::ends_with(spath, ".fq") ||
                 tlx::ends_with(spath, ".fq.gz") ||
                 tlx::ends_with(spath, ".fastq") ||
                 tlx::ends_with(spath, ".fastq.gz")) {
            return FileType::Fastq;
        }
        else if (tlx::ends_with(spath, ".mfasta")) {
            return FileType::FastaMulti;
        }
        else if (tlx::ends_with(spath, ".mfastq")) {
            return FileType::FastqMulti;
        }
        else if (tlx::ends_with(spath, ".list")) {
            return FileType::List;
        }
        else {
            return FileType::Any;
        }
    }

    //! identify and load a DocumentEntry for given path
    static DocumentEntryList load(const fs::path& path) {
        FileType ft = identify_filetype(path);
        if (ft == FileType::Text) {
            DocumentEntry de;
            de.path_ = path.string();
            de.type_ = FileType::Text;
            de.name_ = base_name(path);
            de.size_ = fs::file_size(path);
            de.term_size_ = 0, de.term_count_ = 0;
            return DocumentEntryList({ de });
        }
        else if (ft == FileType::Cortex) {
            CortexFile ctx(path.string());
            DocumentEntry de;
            de.path_ = path.string();
            de.type_ = FileType::Cortex;
            de.name_ = ctx.name_;
            de.size_ = fs::file_size(path);
            de.term_size_ = ctx.kmer_size_;
            de.term_count_ = ctx.num_kmers();
            return DocumentEntryList({ de });
        }
        else if (ft == FileType::KMerBuffer) {
            std::ifstream is;
            KMerBufferHeader dh = deserialize_header<KMerBufferHeader>(is, path);
            DocumentEntry de;
            de.path_ = path.string();
            de.type_ = FileType::KMerBuffer;
            de.name_ = dh.name();
            de.size_ = fs::file_size(path);
            // calculate number of k-mers from file size, k-mers are bit encoded
            size_t size = get_stream_size(is);
            de.term_size_ = dh.kmer_size();
            de.term_count_ = size / ((de.term_size_ + 3) / 4);
            return DocumentEntryList({ de });
        }
        else if (ft == FileType::Fasta) {
            FastaFile fasta(path.string());
            DocumentEntry de;
            de.path_ = path.string();
            de.type_ = FileType::Fasta;
            de.name_ = cobs::base_name(path);
            de.size_ = fasta.size();
            de.term_size_ = 0, de.term_count_ = 0;
            return DocumentEntryList({ de });
        }
        else if (ft == FileType::FastaMulti) {
            DocumentEntryList list;
            FastaMultifile mfasta(path.string());
            for (size_t i = 0; i < mfasta.num_documents(); ++i) {
                DocumentEntry de;
                de.path_ = path.string();
                de.type_ = FileType::FastaMulti;
                de.name_ = cobs::base_name(path) + '_' + pad_index(i);
                de.size_ = mfasta.size(i);
                de.subdoc_index_ = i;
                de.term_size_ = 0, de.term_count_ = 0;
                list.emplace_back(de);
            }
            return list;
        }
        else if (ft == FileType::Fastq) {
            FastqFile fastq(path.string());
            DocumentEntry de;
            de.path_ = path.string();
            de.type_ = FileType::Fastq;
            de.name_ = cobs::base_name(path);
            de.size_ = fastq.size();
            de.term_size_ = 0, de.term_count_ = 0;
            return DocumentEntryList({ de });
        }
        else {
            die("DocumentList: unknown document file to add: " << path);
        }
    }

    //! add file to DocumentList
    void add(const fs::path& path) {
        DocumentEntryList l = load(path);
        std::move(l.begin(), l.end(), std::back_inserter(list_));
    }

    //! scan path recursively and add files of given filter type. if the root
    //! path is a .list file, it is read and all files are added to the
    //! DocumentList.
    void add_recursive(const fs::path& root, FileType filter = FileType::Any)
    {
        std::vector<std::string> paths;
        if (fs::is_directory(root)) {
            fs::recursive_directory_iterator it(root), end;
            while (it != end) {
                if (accept(*it, filter)) {
                    paths.emplace_back(fs::path(*it).string());
                }
                ++it;
            }
        }
        else if (tlx::ends_with(root.string(), ".list") ||
                 filter == FileType::List)
        {
            std::ifstream in(root.string());
            if (!in.good())
                die("DocumentList: could not open .list file: " << root);

            fs::path root_parent = root.parent_path();
            std::string line;
            while (std::getline(in, line)) {
                // skip empty lines and comment lines
                if (line.size() == 0 || line[0] == '#')
                    continue;

                fs::path file(line);
                if (!file.is_absolute()) {
                    // if path line is not absolute, add parent path of the
                    // list file.
                    file = root_parent / file;
                }
                paths.emplace_back(file.string());
            }
        }
        else if (fs::is_regular_file(root)) {
            paths.emplace_back(root.string());
        }

        std::sort(paths.begin(), paths.end());

        // process files in parallel such that caches files are created in
        // parallel
        std::mutex list_mutex;
        parallel_for(
            0, paths.size(), gopt_threads,
            [&](size_t i) {
                try {
                    DocumentEntryList l = load(fs::path(paths[i]));

                    // append the list of documents loaded from the path
                    std::unique_lock<std::mutex> lock(list_mutex);
                    std::move(l.begin(), l.end(), std::back_inserter(list_));
                }
                catch (std::exception& e) {
                    LOG1 << "EXCEPTION: " << e.what();
                }
            });

        // sort again due to random thread execution order
        if (gopt_threads > 1)
            std::sort(list_.begin(), list_.end());
    }

    //! return list of paths
    const DocumentEntryList& list() const { return list_; }

    //! return length of list
    size_t size() const { return list_.size(); }

    //! return DocumentEntry index
    const DocumentEntry& operator [] (size_t i) const {
        return list_.at(i);
    }

    //! sort files by path
    void sort_by_path() {
        std::sort(list_.begin(), list_.end(),
                  [](const DocumentEntry& d1, const DocumentEntry& d2) {
                      return d1.path_ < d2.path_;
                  });
    }

    //! sort files by size
    void sort_by_size() {
        std::sort(list_.begin(), list_.end(),
                  [](const DocumentEntry& d1, const DocumentEntry& d2) {
                      return (std::tie(d1.size_, d1.path_) <
                              std::tie(d2.size_, d2.path_));
                  });
    }

    //! process each file
    void process_each(void (*func)(const DocumentEntry&)) const {
        for (size_t i = 0; i < list_.size(); i++) {
            func(list_[i]);
        }
    }

    //! process files in batch with output file generation
    template <typename Functor>
    void process_batches(size_t batch_size, Functor func) const {
        std::string first_filename, last_filename;
        size_t batch_num = 0;
        DocumentEntryList batch;
        for (size_t i = 0; i < list_.size(); i++) {
            std::string filename = list_[i].name_;
            if (first_filename.empty()) {
                first_filename = filename;
            }
            last_filename = filename;
            batch.push_back(list_[i]);

            if (batch.size() == batch_size ||
                (!batch.empty() && i + 1 == list_.size()))
            {
                std::string out_file =
                    pad_index(batch_num) + '_' +
                    '[' + first_filename + '-' + last_filename + ']';

                func(batch_num, batch, out_file);

                batch.clear();
                first_filename.clear();
                batch_num++;
            }
        }
    }

    //! process files in batch with output file generation
    template <typename Functor>
    void process_batches_parallel(size_t batch_size, size_t num_threads,
                                  Functor func) const {
        struct Batch {
            DocumentEntryList batch;
            std::string out_file;
        };
        std::vector<Batch> batch_list;

        std::string first_filename, last_filename;
        size_t batch_num = 0;
        DocumentEntryList batch;
        for (size_t i = 0; i < list_.size(); i++) {
            std::string filename = list_[i].name_;
            if (first_filename.empty()) {
                first_filename = filename;
            }
            last_filename = filename;
            batch.push_back(list_[i]);

            if (batch.size() == batch_size ||
                (!batch.empty() && i + 1 == list_.size()))
            {
                std::string out_file =
                    pad_index(batch_num) + '_' +
                    '[' + first_filename + '-' + last_filename + ']';

                batch_list.emplace_back(Batch { std::move(batch), out_file });

                first_filename.clear();
                batch_num++;
            }
        }

        parallel_for(
            0, batch_list.size(), num_threads,
            [&](size_t i) {
                func(i, batch_list[i].batch, batch_list[i].out_file);
            });
    }

private:
    DocumentEntryList list_;
};

} // namespace cobs

#endif // !COBS_DOCUMENT_LIST_HEADER

/******************************************************************************/

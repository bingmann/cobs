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

#include <algorithm>
#include <iomanip>

#include <tlx/logger.hpp>

namespace cobs {

enum class FileType {
    Any,
    Text,
    Cortex,
    KMerBuffer,
    Fasta,
    Fastq,
    FastaMulti,
    FastqMulti,
};

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
    //! number of terms
    size_t term_count_ = 0;

    //! default sort operator
    bool operator < (const DocumentEntry& b) const {
        return (std::tie(path_, subdoc_index_) <
                std::tie(b.path_, b.subdoc_index_));
    }

    //! calculate number of terms in file
    size_t num_terms(size_t k) const {
        if (term_size_ == 0) {
            if (type_ == FileType::Fasta) {
                // read fasta index
                FastaFile fasta(path_);
                return fasta.num_terms(k);
            }
            return size_ < k ? 0 : size_ - k + 1;
        }
        die_verbose_unequal(
            term_size_, k, "DocumentEntry term_size_ mismatches requested k");
        return term_count_;
    }

    //! process terms
    template <typename Callback>
    void process_terms(unsigned term_size, Callback callback) const {
        if (type_ == FileType::Text) {
            TextFile text(path_);
            text.process_terms(term_size, callback);
        }
        else if (type_ == FileType::Cortex) {
            die_unequal(term_size, 31u);
            CortexFile ctx(path_);
            ctx.process_terms<31>(callback);
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
                callback(term);
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

    //! accept a file list, sort by name
    DocumentList(const DocumentEntryList& list)
        : list_(list) {
        std::sort(list_.begin(), list_.end());
    }

    //! read a directory, filter files
    DocumentList(const fs::path& root, FileType filter = FileType::Any) {
        std::vector<fs::path> paths;
        if (fs::is_directory(root)) {
            fs::recursive_directory_iterator it(root), end;
            while (it != end) {
                if (accept(*it, filter)) {
                    paths.emplace_back(*it);
                }
                ++it;
            }
        }
        else if (fs::is_regular_file(root)) {
            paths.emplace_back(root);
        }

        std::sort(paths.begin(), paths.end());

#pragma omp parallel for if(gopt_parallel)
        for (size_t i = 0; i < paths.size(); ++i) {
            try {
                DocumentEntryList del = add(paths[i]);
#pragma omp critical
                std::move(del.begin(), del.end(), std::back_inserter(list_));
            }
            catch (std::exception& e) {
                LOG1 << "EXCEPTION: " << e.what();
            }
        }

        // sort again due to random thread execution order
        if (gopt_parallel)
            std::sort(list_.begin(), list_.end());
    }

    //! filter method to match file specifications
    static bool accept(const fs::path& path, FileType filter) {
        switch (filter) {
        case FileType::Any:
            return tlx::ends_with(path, ".txt") ||
                   tlx::ends_with(path, ".ctx") ||
                   tlx::ends_with(path, ".cobs_doc") ||
                   tlx::ends_with(path, ".fasta") ||
                   tlx::ends_with(path, ".fasta.gz") ||
                   tlx::ends_with(path, ".fastq") ||
                   tlx::ends_with(path, ".fastq.gz") ||
                   tlx::ends_with(path, ".mfasta") ||
                   tlx::ends_with(path, ".mfastq");
        case FileType::Text:
            return tlx::ends_with(path, ".txt");
        case FileType::Cortex:
            return tlx::ends_with(path, ".ctx");
        case FileType::KMerBuffer:
            return tlx::ends_with(path, ".cobs_doc");
        case FileType::Fasta:
            return tlx::ends_with(path, ".fasta") ||
                   tlx::ends_with(path, ".fasta.gz");
        case FileType::Fastq:
            return tlx::ends_with(path, ".fastq") ||
                   tlx::ends_with(path, ".fastq.gz");
        case FileType::FastaMulti:
            return tlx::ends_with(path, ".mfasta");
        case FileType::FastqMulti:
            return tlx::ends_with(path, ".mfastq");
        }
        return false;
    }

    //! add DocumentEntry for given file
    DocumentEntryList add(const fs::path& path) const {
        if (tlx::ends_with(path, ".txt")) {
            DocumentEntry de;
            de.path_ = path;
            de.type_ = FileType::Text;
            de.name_ = base_name(path);
            de.size_ = fs::file_size(path);
            de.term_size_ = 0, de.term_count_ = 0;
            return DocumentEntryList({ de });
        }
        else if (tlx::ends_with(path, ".ctx")) {
            CortexFile ctx(path);
            DocumentEntry de;
            de.path_ = path;
            de.type_ = FileType::Cortex;
            de.name_ = ctx.name_;
            de.size_ = fs::file_size(path);
            de.term_size_ = ctx.kmer_size_;
            de.term_count_ = ctx.num_kmers();
            return DocumentEntryList({ de });
        }
        else if (tlx::ends_with(path, ".cobs_doc")) {
            std::ifstream is;
            KMerBufferHeader dh = deserialize_header<KMerBufferHeader>(is, path);
            DocumentEntry de;
            de.path_ = path;
            de.type_ = FileType::KMerBuffer;
            de.name_ = dh.name();
            de.size_ = fs::file_size(path);
            // calculate number of k-mers from file size, k-mers are bit encoded
            size_t size = get_stream_size(is);
            de.term_size_ = dh.kmer_size();
            de.term_count_ = size / ((de.term_size_ + 3) / 4);
            return DocumentEntryList({ de });
        }
        else if (tlx::ends_with(path, ".fasta") ||
                 tlx::ends_with(path, ".fasta.gz")) {
            FastaFile fasta(path);
            DocumentEntry de;
            de.path_ = path;
            de.type_ = FileType::Fasta;
            de.name_ = cobs::base_name(path);
            de.size_ = fasta.size();
            de.term_size_ = 0, de.term_count_ = 0;
            return DocumentEntryList({ de });
        }
        else if (tlx::ends_with(path, ".mfasta")) {
            DocumentEntryList list;
            FastaMultifile mfasta(path);
            for (size_t i = 0; i < mfasta.num_documents(); ++i) {
                DocumentEntry de;
                de.path_ = path;
                de.type_ = FileType::FastaMulti;
                de.name_ = cobs::base_name(path) + '_' + pad_index(i);
                de.size_ = mfasta.size(i);
                de.subdoc_index_ = i;
                de.term_size_ = 0, de.term_count_ = 0;
                list.emplace_back(de);
            }
            return list;
        }
        else if (tlx::ends_with(path, ".fastq") ||
                 tlx::ends_with(path, ".fastq.gz")) {
            FastqFile fastq(path);
            DocumentEntry de;
            de.path_ = path;
            de.type_ = FileType::Fastq;
            de.name_ = cobs::base_name(path);
            de.size_ = fastq.size();
            de.term_size_ = 0, de.term_count_ = 0;
            return DocumentEntryList({ de });
        }
        else {
            die("DocumentList: unknown file to add: " << path);
        }
    }

    //! return list of paths
    const DocumentEntryList& list() const { return list_; }

    //! return length of list
    size_t size() const { return list_.size(); }

    //! return DocumentEntry index
    const DocumentEntry& operator [] (size_t i) {
        return list_.at(i);
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
    void process_batches_parallel(size_t batch_size, Functor func) const {
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

#pragma omp parallel for schedule(dynamic) if(gopt_parallel)
        for (size_t i = 0; i < batch_list.size(); ++i) {
            func(i, batch_list[i].batch, batch_list[i].out_file);
        }
    }

private:
    DocumentEntryList list_;
};

} // namespace cobs

#endif // !COBS_DOCUMENT_LIST_HEADER

/******************************************************************************/

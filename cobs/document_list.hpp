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
};

/*!
 * DocumentEntry specifies a document or subdocument which can deliver a set of
 * q-grams for indexing.
 */
struct DocumentEntry {
    //! file system path to document
    fs::path path_;
    //! type of document
    FileType type_;
    //! name of the document
    std::string name_;
    //! size of the document in bytes
    size_t size_;
    //! subdocument index (for FASTA, FASTQ, etc)
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
            die_unequal(term_size, 31);
            CortexFile ctx(path_);
            ctx.process_terms<31>(callback);
        }
        else if (type_ == FileType::KMerBuffer) {
            die_unequal(term_size, 31);
            KMerBuffer<31> doc;
            KMerBufferHeader dh;
            doc.deserialize(path_, dh);

            std::string term;
            term.reserve(32);

            for (uint64_t j = 0; j < doc.data().size(); j++) {
                doc.data()[j].canonicalize();
                doc.data()[j].to_string(&term);
                callback(term);
            }
        }
        else if (type_ == FileType::Fasta) {
            FastaFile fasta(path_);
            fasta.process_terms(subdoc_index_, term_size, callback);
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
    //! accept a file list, sort by name
    DocumentList(const std::vector<DocumentEntry>& list)
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
        for (const fs::path& p : paths) {
            add(p);
        }
    }

    //! filter method to match file specifications
    static bool accept(const fs::path& path, FileType filter) {
        switch (filter) {
        case FileType::Any:
            return path.extension() == ".txt" ||
                   path.extension() == ".ctx" ||
                   path.extension() == ".cobs_doc" ||
                   path.extension() == ".fasta" ||
                   path.extension() == ".fastq";
        case FileType::Text:
            return path.extension() == ".txt";
        case FileType::Cortex:
            return path.extension() == ".ctx";
        case FileType::KMerBuffer:
            return path.extension() == ".cobs_doc";
        case FileType::Fasta:
            return path.extension() == ".fasta";
        case FileType::Fastq:
            return path.extension() == ".fastq";
        }
        return false;
    }

    //! add DocumentEntry for given file
    void add(const fs::path& path) {
        if (path.extension() == ".txt") {
            DocumentEntry de;
            de.path_ = path;
            de.type_ = FileType::Text;
            de.name_ = base_name(path);
            de.size_ = fs::file_size(path);
            de.term_size_ = 0, de.term_count_ = 0;
            list_.emplace_back(de);
        }
        else if (path.extension() == ".ctx") {
            CortexFile ctx(path);
            DocumentEntry de;
            de.path_ = path;
            de.type_ = FileType::Cortex;
            de.name_ = ctx.name_;
            de.size_ = fs::file_size(path);
            de.term_size_ = ctx.kmer_size_;
            de.term_count_ = ctx.num_kmers();
            list_.emplace_back(de);
        }
        else if (path.extension() == ".cobs_doc") {
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
            list_.emplace_back(de);
        }
        else if (path.extension() == ".fasta") {
            FastaFile fasta(path);
            for (size_t i = 0; i < fasta.num_documents(); ++i) {
                DocumentEntry de;
                de.path_ = path;
                de.type_ = FileType::Fasta;
                de.name_ = cobs::base_name(path) + '_' + pad_index(i);
                de.size_ = fasta.size(i);
                de.subdoc_index_ = i;
                de.term_size_ = 0, de.term_count_ = 0;
                list_.emplace_back(de);
            }
        }
    }

    //! return list of paths
    const std::vector<DocumentEntry>& list() const { return list_; }

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
        std::vector<DocumentEntry> batch;
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

                LOG1 << "IN - " << out_file;
                func(batch, out_file);
                LOG1 << "OK - " << out_file;

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
            std::vector<DocumentEntry> files;
            std::string out_file;
        };
        std::vector<Batch> batch_list;

        std::string first_filename, last_filename;
        size_t batch_num = 0;
        std::vector<DocumentEntry> batch;
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

#pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < batch_list.size(); ++i) {
            LOG1 << "IN - " << batch_list[i].out_file;
            func(batch_list[i].files, batch_list[i].out_file);
            LOG1 << "OK - " << batch_list[i].out_file;
        }
    }

private:
    std::vector<DocumentEntry> list_;
};

} // namespace cobs

#endif // !COBS_DOCUMENT_LIST_HEADER

/******************************************************************************/

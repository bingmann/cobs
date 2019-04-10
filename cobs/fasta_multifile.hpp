/*******************************************************************************
 * cobs/fasta_multifile.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FASTA_MULTIFILE_HEADER
#define COBS_FASTA_MULTIFILE_HEADER

#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/serialization.hpp>
#include <cobs/util/string_view.hpp>
#include <cobs/util/thread_object_array.hpp>

#include <tlx/container/lru_cache.hpp>
#include <tlx/die.hpp>
#include <tlx/logger.hpp>

namespace cobs {

//! metadata structure for Fasta sub-documents
class FastaSubfile
{
public:
    FastaSubfile(std::string path, std::string name,
                 std::istream::pos_type pos_begin,
                 size_t size,
                 const ThreadObjectArrayPtr<std::ifstream>& ifstream_array)
        : path_(path), name_(name), pos_begin_(pos_begin), size_(size),
          ifstream_array_(ifstream_array) { }

    template <typename Callback>
    void process_terms(size_t term_size, Callback callback) {
        auto is = ifstream_array_->get(path_);

        is->clear();
        is->seekg(pos_begin_);
        die_unless(is->good());

        std::string data, line;

        while (std::getline(*is, line)) {
            if (line[0] == '>' || line[0] == ';')
                break;

            data += line;
            if (data.empty())
                continue;

            for (size_t i = 0; i + term_size <= data.size(); ++i) {
                callback(string_view(data.data() + i, term_size));
            }
            data.erase(0, data.size() - term_size + 1);
        }
    }

    //! Returns name_
    const std::string& name() const { return name_; }

    //! Returns pos_begin_
    uint64_t pos_begin() const { return pos_begin_; }

    //! Returns size_
    size_t size() const { return size_; }

private:
    //! file path
    std::string path_;
    //! description from > header
    std::string name_;
    //! start position in fasta file
    std::istream::pos_type pos_begin_;
    //! length of the sequence
    size_t size_;
    //! file handle thread array
    ThreadObjectArrayPtr<std::ifstream> ifstream_array_;
};

using FastaSubfileList = std::vector<FastaSubfile>;
using FastaSubfileListPtr = std::shared_ptr<FastaSubfileList>;

class FastaIndexCache
{
public:
    void put(const std::string& path, const FastaSubfileListPtr& list) {
        std::unique_lock<std::mutex> lock(mutex_);
        lru_.put(path, list);
    }

    bool lookup(const std::string& path, FastaSubfileListPtr& list) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!lru_.exists(path))
            return false;
        list = lru_.get_touch(path);
        return true;
    }

private:
    //! LRU cache
    tlx::LruCacheMap<std::string, FastaSubfileListPtr> lru_;
    //! lock
    std::mutex mutex_;
};

class FastaMultifile
{
public:
    FastaMultifile(std::string path, bool use_cache = true) {
        std::ifstream is(path);
        die_unless(is.good());

        char first = is.get();
        if (first != '>' && first != ';')
            die("FastaMultifile: file does not start with > or ;");

        ifstream_array_ =
            std::make_shared<ThreadObjectArray<std::ifstream> >(lru_set_);

        if (!use_cache) {
            compute_index(path, is);
        }
        else if (cache_.lookup(path, index_)) {
            // all good.
        }
        else if (read_cache_file(path)) {
            cache_.put(path, index_);
        }
        else {
            compute_index(path, is);
            write_cache_file(path);
            cache_.put(path, index_);
        }
    }

    //! read complete FASTA file for sub-documents
    void compute_index(std::string path, std::ifstream& is) {
        LOG1 << "FastaMultifile: computing index for " << path;

        is.clear();
        is.seekg(0);
        index_ = std::make_shared<FastaSubfileList>();

        std::string line;
        // read first line
        std::getline(is, line);

        do {
            if (line.size() == 0 || line[0] == ';') {
                // ; comment header
                std::getline(is, line);
            }
            else if (line[0] == '>') {
                // > document header
                std::string name = line;
                std::istream::pos_type pos_begin = is.tellg();
                size_t size = 0;

                if (name.size() > 16)
                    name.resize(16);

                while (std::getline(is, line)) {
                    if (line[0] == '>' || line[0] == ';')
                        break;

                    size += line.size();
                }
                index_->emplace_back(
                    FastaSubfile(path, name, pos_begin, size,
                                 ifstream_array_));
                // next line is already read
            }
            else if (line[0] == '\r') {
                // skip newline
                std::getline(is, line);
            }
            else {
                std::cout << "fasta: invalid line " << line << std::endl;
                std::getline(is, line);
            }
        }
        while (is.good());
    }

    //! return index cache file path
    std::string cache_path(std::string path) {
        return path + ".cobs_cache";
    }

    //! write cache file
    void write_cache_file(std::string path) {
        std::ofstream os(cache_path(path) + ".tmp");
        stream_put_pod(os, index_->size());
        for (size_t i = 0; i < index_->size(); ++i) {
            stream_put_pod(os, (*index_)[i].size());
            stream_put_pod(os, (*index_)[i].pos_begin());
            os << (*index_)[i].name() << '\0';
        }
        fs::rename(cache_path(path) + ".tmp",
                   cache_path(path));
        LOG1 << "FastaMultifile: saved index as " << cache_path(path);
    }

    //! read cache file
    bool read_cache_file(std::string path) {
        std::ifstream is(cache_path(path));
        if (!is.good()) return false;
        size_t list_size;
        stream_get_pod(is, list_size);
        LOG1 << "FastaMultifile: loading index " << cache_path(path)
             << " [" << list_size << " documents]";
        index_ = std::make_shared<FastaSubfileList>();
        for (size_t i = 0; i < list_size; ++i) {
            size_t size;
            uint64_t pos_begin;
            std::string name;

            stream_get_pod(is, size);
            stream_get_pod(is, pos_begin);
            std::getline(is, name, '\0');

            index_->emplace_back(path, name, pos_begin, size,
                                 ifstream_array_);
        }
        return is.good() && (is.get() == EOF);
    }

    //! return number of sub-documents
    size_t num_documents() const {
        return index_->size();
    }

    //! return size of a sub-document
    size_t size(size_t doc_index) const {
        if (doc_index >= index_->size())
            return 0;
        return (*index_)[doc_index].size();
    }

    template <typename Callback>
    void process_terms(size_t doc_index, size_t term_size, Callback callback) {
        if (doc_index >= index_->size())
            return;
        (*index_)[doc_index].process_terms(term_size, callback);
    }

    //! index of sub-documents
    FastaSubfileListPtr index_;

private:
    //! file stream array
    ThreadObjectArrayPtr<std::ifstream> ifstream_array_;
    //! global index cache
    static FastaIndexCache cache_;
    //! file handle LRU
    static ThreadObjectLRUSet<std::ifstream> lru_set_;
};

} // namespace cobs

#endif // !COBS_FASTA_MULTIFILE_HEADER

/******************************************************************************/

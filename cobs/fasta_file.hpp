/*******************************************************************************
 * cobs/fasta_file.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FASTA_FILE_HEADER
#define COBS_FASTA_FILE_HEADER

#include <cstring>
#include <string>
#include <vector>

#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/string_view.hpp>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <map>

namespace cobs {

class FastaFile
{
public:
    FastaFile(std::string path, bool use_cache = true) : path_(path) {
        is_.open(path);
        die_unless(is_.good());

        std::string line;
        std::getline(is_, line);
        die_unless(is_.good());

        if (line.empty() || (line[0] != '>' && line[0] != ';'))
            die("FastaFile: file does not start with > or ; - " << path);

        if (!use_cache) {
            compute_index();
        }
        else if (read_cache_file()) {
            // all good.
        }
        else {
            compute_index();
            write_cache_file();
        }
    }

    //! read complete FASTA file for sub-documents
    void compute_index() {
        LOG1 << "FastaFile: computing index for " << path_;

        is_.clear();
        is_.seekg(0);

        char line[64 * 1024];
        size_t sequence_size = 0;
        sequence_count_ = 0;

        while (is_.getline(line, sizeof(line))) {
            if (is_.gcount() == 0 || line[0] == '>' || line[0] == ';') {
                // comment or empty line restart the term buffer
                if (sequence_size != 0) {
                    sequence_size_hist_[sequence_size]++;
                    sequence_count_++;
                }
                sequence_size = 0;
                continue;
            }
            sequence_size += is_.gcount();
        }
        if (sequence_size != 0) {
            sequence_size_hist_[sequence_size]++;
            sequence_count_++;
        }
    }

    //! return index cache file path
    std::string cache_path() const {
        return path_ + ".cobs_cache";
    }

    //! write cache file
    void write_cache_file() {
        std::ofstream os(cache_path() + ".tmp");
        stream_put_pod(os, sequence_count_);
        stream_put_pod(os, sequence_size_hist_.size());
        for (auto it = sequence_size_hist_.begin();
             it != sequence_size_hist_.end(); ++it) {
            stream_put_pod(os, it->first);
            stream_put_pod(os, it->second);
        }
        fs::rename(cache_path() + ".tmp",
                   cache_path());
        LOG1 << "FastaFile: saved index as " << cache_path();
    }

    //! read cache file
    bool read_cache_file() {
        std::ifstream is(cache_path());
        if (!is.good()) return false;
        size_t hist_size;
        stream_get_pod(is, sequence_count_);
        stream_get_pod(is, hist_size);
        LOG1 << "FastaFile: loading index " << cache_path()
             << " [" << sequence_count_ << " subsequences]";
        for (size_t i = 0; i < hist_size; ++i) {
            size_t size, count;
            stream_get_pod(is, size);
            stream_get_pod(is, count);
            sequence_size_hist_[size] = count;
        }
        return is.good() && (is.get() == EOF);
    }

    //! return estimated size of a fasta document
    size_t size() {
        is_.clear();
        is_.seekg(0, std::ios::end);
        return is_.tellg();
    }

    //! return number of q-grams in document
    size_t num_terms(size_t q) {
        size_t total = 0;
        for (const auto& p : sequence_size_hist_) {
            total += p.second * (p.first < q ? 0 : p.first - q + 1);
        }
        return total;
    }

    template <typename Callback>
    void process_terms(size_t term_size, Callback callback) {
        is_.clear();
        is_.seekg(0);
        die_unless(is_.good());

        char line[64 * 1024];
        std::string data;

        while (is_.getline(line, sizeof(line))) {
            if (is_.gcount() == 0 || line[0] == '>' || line[0] == ';') {
                // comment or empty line restart the term buffer
                data.clear();
                continue;
            }

            // process terms continued on next line
            data.append(line, is_.gcount());
            for (size_t i = 0; i + term_size <= data.size(); ++i) {
                callback(string_view(data.data() + i, term_size));
            }
            data.erase(0, data.size() - term_size + 1);
        }
    }

private:
    //! file stream
    std::ifstream is_;
    //! path
    std::string path_;
    //! number of sub-sequences
    size_t sequence_count_;
    //! histogram of sub-sequence sizes
    std::map<size_t, size_t> sequence_size_hist_;
};

} // namespace cobs

#endif // !COBS_FASTA_FILE_HEADER

/******************************************************************************/

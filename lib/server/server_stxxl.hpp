#pragma once


#include <stxxl/io>
#include <stxxl/aligned_alloc>
#include "server.hpp"

namespace genome {
    class server_stxxl : public server {
    private:
        stxxl::syscall_file m_file;
        size_t m_header_end;

        void read_from_disk(const std::vector<size_t>& hashes, char* rows) {
            m_timer.active("file_access");

            std::vector<stxxl::request_ptr> requests(hashes.size());

            for (size_t i = 0; i < hashes.size(); i++) {
                requests[i] = m_file.aread(rows, hashes[i] * m_bfh.block_size(), m_bfh.block_size());
            }

            stxxl::wait_all(requests.data(), requests.size());
        }
    protected:
        void get_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) override {
            std::vector<byte> count(m_bfh.block_size(), 0xFF);
            auto* count_64 = reinterpret_cast<uint64_t*>(count.data());
            auto* counts_64 = reinterpret_cast<uint64_t*>(counts.data());

            auto* rows = reinterpret_cast<char*>(stxxl::aligned_alloc<4096>(m_bfh.block_size() * hashes.size()));
            read_from_disk(hashes, rows);

            m_timer.active("compute_counts");

            #pragma omp parallel for
            for (size_t i = 0; i < hashes.size(); i++) {
                auto rows_64 = reinterpret_cast<uint64_t*>(rows);
                size_t j = 1;
                while (j * 8 <= m_bfh.block_size()) {
                    count_64[j - 1] &= rows_64[j - 1];
                    j++;
                }
                j = (j - 1) * 8;
                while (j < m_bfh.block_size()) {
                    count[j] &= rows[j];
                    j++;
                }
                if (i % m_bfh.num_hashes() == m_bfh.num_hashes() - 1) {
                    for (size_t k = 0; k < m_bfh.block_size(); k++) {
                        counts_64[2 * k] += m_count_expansions[count[k] & 0xF];
                        counts_64[2 * k + 1] += m_count_expansions[count[k] >> 4];
                    }
                    std::fill(count.begin(), count.end(), 0xFF);
                }
                rows += m_bfh.block_size();
            }
            stxxl::aligned_dealloc<4096>(rows);
        }

    public:
        explicit server_stxxl(const boost::filesystem::path& path) : m_file(path.string(), stxxl::file::RDONLY) {
            std::ifstream ifs;
            m_bfh = file::deserialize_header<file::bloom_filter_header>(ifs, path);
            stream_metadata smd = get_stream_metadata(ifs);
            m_header_end = smd.curr_pos;
        }
    };
}

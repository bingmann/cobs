#include <fstream>
#include <file/util.hpp>
#include <fcntl.h>
#include <sys/mman.h>
#include "server.hpp"

namespace genome {
    server::server(const boost::filesystem::path& path) {
        m_bfh = file::deserialize_header<file::bloom_filter_header>(m_ifs, path);
        m_pos_data_beg = m_ifs.tellg();
        m_ifs.seekg(0, std::ios::end);
        size_t size = m_ifs.tellg();
        m_ifs.close();


        int fd = open(path.string().data(), O_RDONLY, 0);
        assert(fd != -1);

//  | MAP_POPULATE
        m_data = m_pos_data_beg + reinterpret_cast<byte*>(mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0));
    }

    server::server(const boost::filesystem::path& path, int a) {
        m_bfh = file::deserialize_header<file::bloom_filter_header>(m_ifs, path);
        m_pos_data_beg = m_ifs.tellg();
    }

    template<unsigned int N>
    void server::create_hashes(std::vector<size_t>& hashes, const std::string& query) {
        hashes.clear();
        hashes.reserve(m_bfh.num_hashes() * (query.size() - N + 1));
        kmer<31> k;
        for (size_t i = 0; i <= query.size() - N; i++) {
            k.init(query.data() + i);
            bloom_filter::create_hashes(k.data().data(), kmer<31>::size, m_bfh.bloom_filter_size(), m_bfh.num_hashes(),
                                        [&](size_t hash) {
                                            hashes.push_back(hash);
                                        });
        }
    }


    template<unsigned int N>
    void server::search_bloom_filter(const std::string& query, std::vector<std::pair<std::string, double>>& result) {
        std::vector<size_t> hashes;
        create_hashes<N>(hashes, query);

        std::vector<byte> count(m_bfh.block_size());
        std::vector<size_t> counts(8 * m_bfh.block_size());
        for (size_t i = 0; i < hashes.size(); i++) {
            for (size_t j = 0; j < m_bfh.block_size(); j++) {
                count[j] &= m_data[hashes[i] * m_bfh.block_size() + j];
            }
            if (i % m_bfh.num_hashes() == m_bfh.num_hashes() - 1) {
                for (size_t j = 0; j < m_bfh.block_size(); j++) {
                    for (size_t k = 0; k < 8; k++) {
                        counts[8 * j + k] += (count[j] >> k) & 1;
                    }
                }
                std::fill(count.begin(), count.end(), 1);
            }
        }

        result.clear();
        result.reserve(counts.size());
        for (size_t i = 0; i < counts.size(); i++) {
            result.emplace_back(std::make_pair(m_bfh.file_names()[i], ((double) counts[i]) / hashes.size()));
        }
        std::sort(result.begin(), result.end(), [&](const auto& p1, const auto& p2){
            return p1.second > p2.second;
        });
    }

    template<unsigned int N>
    void server::search_bloom_filter_2(const std::string& query, std::vector<std::pair<std::string, double>>& result) {
        std::vector<size_t> hashes;
        create_hashes<N>(hashes, query);

        bloom_filter bf(hashes.size(), m_bfh.block_size(), m_bfh.num_hashes());
        auto data_ptr = reinterpret_cast<char*>(bf.data().data());
        for (size_t i = 0; i < hashes.size(); i++) {
            m_ifs.seekg(m_pos_data_beg + m_bfh.block_size() * hashes[i]);
            m_ifs.read(data_ptr + i * m_bfh.block_size(), m_bfh.block_size());
        }

        std::vector<byte> count(bf.block_size());
        std::vector<size_t> counts(8 * bf.block_size());
        for (size_t i = 0; i < hashes.size(); i++) {
            for (size_t j = 0; j < bf.block_size(); j++) {
                count[j] &= bf.data()[i * bf.block_size() + j];
            }
            if (i % bf.num_hashes() == bf.num_hashes() - 1) {
                for (size_t j = 0; j < bf.block_size(); j++) {
                    for (size_t k = 0; k < 8; k++) {
                        counts[8 * j + k] += (count[j] >> k) & 1;
                    }
                }
                std::fill(count.begin(), count.end(), 1);
            }
        }

        result.clear();
        result.reserve(counts.size());
        for (size_t i = 0; i < counts.size(); i++) {
            result.emplace_back(std::make_pair(m_bfh.file_names()[i], ((double) counts[i]) / hashes.size()));
        }
        std::sort(result.begin(), result.end(), [&](const auto& p1, const auto& p2){
            return p1.second > p2.second;
        });
    }
}

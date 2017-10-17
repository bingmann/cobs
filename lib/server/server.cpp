#include "server.hpp"
#include <kmer.hpp>
#include <bloom_filter.hpp>
#include <numeric>

namespace genome {
    const uint64_t server::m_count_expansions[] = {0, 1, 65536, 65537, 4294967296, 4294967297, 4295032832, 4295032833, 281474976710656,
                                                   281474976710657, 281474976776192, 281474976776193, 281479271677952, 281479271677953,
                                                   281479271743488, 281479271743489};

    void server::create_hashes(std::vector <size_t>& hashes, const std::string& query, uint32_t kmer_size) const {
        hashes.reserve(m_bfh.num_hashes() * (query.size() - kmer_size + 1));
        hashes.clear();
        std::vector<char> kmer_data(kmer<31>::data_size(kmer_size));
        for (size_t i = 0; i <= query.size() - kmer_size; i++) {
            kmer<31>::init(query.data() + i, kmer_data.data(), kmer_size);
            bloom_filter::create_hashes(kmer_data.data(), kmer<31>::data_size(kmer_size), m_bfh.bloom_filter_size(), m_bfh.num_hashes(),
                                        [&](size_t hash) {
                                            hashes.push_back(hash);
                                        });
        }
    }

    void server::counts_to_result(const std::vector<uint16_t>& counts, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) const {
        std::vector<uint32_t> sorted_indices(counts.size());
        std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
        std::nth_element(sorted_indices.begin(), sorted_indices.begin() + num_results, sorted_indices.end(), [&](const auto v1, const auto v2){
            return counts[v1] > counts[v2];
        });

        result.reserve(num_results);
        result.clear();
        for (size_t i = 0; i < num_results; i++) {
            result.emplace_back(std::make_pair(counts[sorted_indices[i]], m_bfh.file_names()[sorted_indices[i]]));
        }
    };

    void server::search_bloom_filter(const std::string& query, uint32_t kmer_size, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) {
        assert(query.size() >= kmer_size);
        assert(query.size() - kmer_size + 1 <= UINT16_MAX);
        m_timer.active("hashes");
        std::vector<size_t> hashes;
        create_hashes(hashes, query, kmer_size);
        m_timer.stop();

        std::vector<uint16_t> counts(8 * m_bfh.block_size());
        get_counts(hashes, counts);
        m_timer.active("counts_to_result");
        counts_to_result(counts, result, num_results == 0 ? m_bfh.file_names().size() : num_results);
        m_timer.stop();
    }

    const timer& server::get_timer() const {
        return m_timer;
    }
}

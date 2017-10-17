#include <fstream>
#include <file/util.hpp>
#include "server.hpp"

namespace genome {
    template<unsigned int N>
    void server::create_hashes(std::vector<size_t>& hashes, const std::string& query) const {
        hashes.reserve(m_bfh.num_hashes() * (query.size() - N + 1));
        hashes.clear();
        kmer<N> k;
        for (size_t i = 0; i <= query.size() - N; i++) {
            k.init(query.data() + i);
            bloom_filter::create_hashes(k.data().data(), kmer<N>::size, m_bfh.bloom_filter_size(), m_bfh.num_hashes(),
                                        [&](size_t hash) {
                                            hashes.push_back(hash);
                                        });
        }
    }

    template<unsigned int N>
    void server::search_bloom_filter(const std::string& query, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) {
        assert(query.size() >= N);
        assert(query.size() - N + 1 <= UINT16_MAX);
        m_timer.active("hashes");
        std::vector<size_t> hashes;
        create_hashes<N>(hashes, query);
        m_timer.stop();

        std::vector<uint16_t> counts(8 * m_bfh.block_size());
        get_counts(hashes, counts);
        m_timer.active("counts_to_result");
        counts_to_result(counts, result, num_results == 0 ? m_bfh.file_names().size() : num_results);
        m_timer.stop();
    }
}

#include <assert.h>
#include "base.hpp"

namespace genome::server::abss {
    void base::search(const std::string& query, uint32_t kmer_size,
                          std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) {
        assert(query.size() >= kmer_size);
        assert(query.size() - kmer_size + 1 <= UINT16_MAX);
        m_timer.active("hashes");
        std::vector<size_t> hashes;
        create_hashes(hashes, query, kmer_size, UINT64_MAX, m_num_hashes);
        m_timer.stop();

        std::vector<uint16_t> counts(8 * m_header.parameters().size() * m_header.page_size());
//        get_counts(hashes, counts);
        m_timer.active("counts_to_result");
        counts_to_result(m_header.file_names(), counts, result, num_results == 0 ? m_header.file_names().size() : num_results);
        m_timer.stop();
    }

    base::base(const boost::filesystem::path& path) : server::base<file::abss_header>(path) {
        //todo assertions that all the data in the header is correct
        m_num_hashes = m_header.parameters()[0].num_hashes;
        for (const auto& p: m_header.parameters()) {
            assert(m_num_hashes == p.num_hashes);
        }
    }
}

#include "base.hpp"
#include <kmer.hpp>
#include <bit_sliced_signatures/bss.hpp>
#include <file/util.hpp>

namespace genome::server::bss {
    void base::search(const std::string& query, uint32_t kmer_size, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) {
        assert(query.size() >= kmer_size);
        assert(query.size() - kmer_size + 1 <= UINT16_MAX);
        m_timer.active("hashes");
        std::vector<size_t> hashes;
        create_hashes(hashes, query, kmer_size, m_header.signature_size(), m_header.num_hashes());
        m_timer.stop();

        std::vector<uint16_t> counts(8 * m_header.block_size());
        get_counts(hashes, counts);
        m_timer.active("counts_to_result");
        counts_to_result(m_header.file_names(), counts, result, num_results == 0 ? m_header.file_names().size() : num_results);
        m_timer.stop();
    }

    base::base(const boost::filesystem::path& path) : server::base<file::bss_header>(path) { }
}

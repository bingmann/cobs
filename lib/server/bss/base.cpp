#include "base.hpp"
#include <kmer.hpp>
#include <bit_sliced_signatures/bss.hpp>
#include <file/util.hpp>

namespace isi::server::bss {
    base::base(const std::experimental::filesystem::path& path) : server::base<file::bss_header>(path) { }

    uint64_t base::num_hashes() const {
            return m_header.num_hashes();
    }

    uint64_t base::block_size() const {
        return m_header.block_size();
    }

    uint64_t base::max_hash_value() const {
            return m_header.signature_size();
    }

    uint64_t base::counts_size() const {
            return 8 * m_header.block_size();
    }
}

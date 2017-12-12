#include <isi/query/classic_index/mmap.hpp>
#include <isi/util/file.hpp>
#include <isi/util/query.hpp>
#include <cstring>

namespace isi::query::classic_index {

    mmap::mmap(const std::experimental::filesystem::path& path) : classic_index::base(path) {
        std::pair<int, uint8_t*> handles = initialize_mmap(path, m_smd);
        m_fd = handles.first;
        m_data = handles.second;
    }

    mmap::~mmap() {
        destroy_mmap(m_fd, m_data, m_smd);
    }

    void mmap::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        #pragma omp parallel for
        for (size_t i = 0; i < hashes.size(); i++) {
            auto data_8 = m_data + hashes[i] % m_header.signature_size() * m_header.block_size();
            auto rows_8 = rows + i * m_header.block_size();
            std::memcpy(rows_8, data_8, m_header.block_size());
        }
    }
}

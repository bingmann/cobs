#include <isi/util/query.hpp>
#include <isi/query/compact_index/mmap.hpp>

namespace isi::query::compact_index {
    mmap::mmap(const std::experimental::filesystem::path& path) : compact_index::base(path) {
        m_data.reserve(m_header.parameters().size());
        std::pair<int, uint8_t*> handles = initialize_mmap(path, m_smd);
        m_fd = handles.first;
        m_data[0] = handles.second;
        for (size_t i = 1; i < m_header.parameters().size(); i++) {
            m_data[i] = m_data[i - 1] + m_header.page_size() * m_header.parameters()[i - 1].signature_size;
        }
    }

    mmap::~mmap() {
        destroy_mmap(m_fd, m_data[0], m_smd);
    }

    void mmap::read_from_disk(const std::vector<size_t>& hashes, char* rows) {
        #pragma omp parallel for collapse(2)
        for (size_t i = 0; i < m_header.parameters().size(); i++) {
            for (size_t j = 0; j < hashes.size(); j++) {
                //todo this is wrong, two times modulo results in a different line?!
                //works because max uint should still change since confusing
                uint64_t hash = hashes[j] % m_header.parameters()[i].signature_size;
                auto data_8 = m_data[i] + hash * m_header.page_size();
                auto rows_8 = rows + (i + j * m_header.parameters().size()) * m_header.page_size();
                std::memcpy(rows_8, data_8, m_header.page_size());
            }
        }
    }
}

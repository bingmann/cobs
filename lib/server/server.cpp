#include <boost/filesystem/path.hpp>
#include <functional>
#include <numeric>
#include "server.hpp"

namespace genome {
    const uint64_t server::m_count_expansions[] = {0, 1, 65536, 65537, 4294967296, 4294967297, 4295032832, 4295032833, 281474976710656,
                                                   281474976710657, 281474976776192, 281474976776193, 281479271677952, 281479271677953,
                                                   281479271743488, 281479271743489};

    void server::counts_to_result(const std::vector<uint16_t>& counts, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) const {

        std::vector<uint32_t> sorted_indices(counts.size());
        std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
        std::sort(sorted_indices.begin(), sorted_indices.end(), [&](const auto v1, const auto v2){
            return counts[v1] > counts[v2];
        });

        result.reserve(num_results);
        result.clear();
        for (size_t i = 0; i < num_results; i++) {
            result.emplace_back(std::make_pair(counts[sorted_indices[i]], m_bfh.file_names()[sorted_indices[i]]));
        }
    }

    const timer& server::get_timer() const {
        return m_timer;
    }
}

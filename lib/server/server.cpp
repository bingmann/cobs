#include <boost/filesystem/path.hpp>
#include <functional>
#include "server.hpp"

namespace genome {
    const uint64_t server::m_count_expansions[] = {0, 1, 65536, 65537, 4294967296, 4294967297, 4295032832, 4295032833, 281474976710656,
                                                   281474976710657, 281474976776192, 281474976776193, 281479271677952, 281479271677953,
                                                   281479271743488, 281479271743489};

    void server::counts_to_result(const std::vector<uint16_t>& counts, size_t max_count, std::vector<std::pair<uint16_t, std::string>>& result) const {
        result.clear();
        result.reserve(counts.size());
        for (size_t i = 0; i < counts.size(); i++) {
            result.emplace_back(std::make_pair(counts[i], m_bfh.file_names()[i]));
        }
        std::sort(result.begin(), result.end(), std::greater<>());
    }

    const timer& server::timer() const {
        return m_timer;
    }
}

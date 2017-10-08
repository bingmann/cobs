#include <boost/filesystem/path.hpp>
#include "server.hpp"

namespace genome {
    void server::counts_to_result(const std::vector <size_t>& counts, size_t max_count, std::vector <std::pair<double, std::string>>& result) const {
        result.clear();
        result.reserve(counts.size());
        for (size_t i = 0; i < counts.size(); i++) {
            result.emplace_back(std::make_pair(((double) counts[i]) / max_count, m_bfh.file_names()[i]));
        }
        std::sort(result.begin(), result.end());
    }

    const timer& server::timer() const {
        return m_timer;
    }
}

#include <cobs/util/processing.hpp>

namespace cobs {
    void get_sorted_file_names(const std::experimental::filesystem::path& in_dir,
                               const std::experimental::filesystem::path& out_dir,
                               std::vector<std::experimental::filesystem::path>& paths) {
        std::experimental::filesystem::create_directories(out_dir);
        std::experimental::filesystem::recursive_directory_iterator it(in_dir), end;
        std::copy(it, end, std::back_inserter(paths));
        std::sort(paths.begin(), paths.end());
    }
}

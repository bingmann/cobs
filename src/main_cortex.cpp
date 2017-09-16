#include <cortex.hpp>

int main(int argc, char** argv) {
    std::string file_paths_string = "/Users/Flo/projects/thesis/data/raw/DRR002056/cleaned/DRR002056.ctx";

    std::vector<std::string> file_paths;
    boost::split(file_paths, file_paths_string, boost::is_any_of(" "));

    clock_t begin = clock();
    sample<31> sample;
    for (const auto& file_path: file_paths) {
//        process_file(boost::filesystem::path(file_path), sample);
    }
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << elapsed_secs << std::endl;
}

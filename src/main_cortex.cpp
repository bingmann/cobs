#include <string>
#include <vector>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>

#include <helpers.hpp>
#include <sample.hpp>

template<unsigned int N>
void format_sample(const std::string& cmd, sample<N>& sample) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        sample.init([&](char* buffer, unsigned int buffer_size) {
            return !feof(pipe) && fgets(buffer, buffer_size, pipe) != nullptr;
        });
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
}

template<unsigned int N>
void serialize(const sample<N>& sample, const std::string& path) {
    std::ofstream outfile(path, std::ios::out | std::ios::binary);
    outfile.write(reinterpret_cast<const char*>(sample.data().data()), sizeof(sample.data()[0]) * sample.data().size());
}

void bulk_process(std::string file_paths_string) {
    std::vector<std::string> file_paths;
    boost::split(file_paths, file_paths_string, boost::is_any_of(" "));

    clock_t begin = clock();
    sample<31> sample;
    for (const auto& file_path: file_paths) {
        std::string file_name = file_path.substr(file_path.find('/') + 1);
        file_name = file_name.substr(0, file_name.find('/'));
        std::string cmd = "mccortex31 view -q -k /Users/Flo/projects/thesis/data/" + file_path;
        format_sample(cmd, sample);
        serialize(sample, "/Users/Flo/projects/thesis/data/" + file_name + ".b");
    }
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << elapsed_secs << std::endl;
}

int main(int argc, char** argv) {
    bulk_process("raw/DRR002056/cleaned/DRR002056.ctx");
}

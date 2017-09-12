#include <iostream>

#include "../lib/sample.hpp"
#include "../lib/helpers.hpp"


template<typename K, typename V>
void write_frequencies(const std::unordered_map<K, V>& frequencies, const std::string& file_name) {

    std::vector<std::pair<K, V>> values(frequencies.begin(), frequencies.end());
    std::sort(values.begin(), values.end());

    std::ofstream outfile(file_name);
    for (const auto& value: values) {
        outfile << value.first << "," << value.second << "\n";
    }
    outfile.close();
}

int main(int argc, char* argv[]) {
    size_t num_kmers;
    std::unordered_map<uint64_t, unsigned int> frequencies;
    int i = 0;
    for (const auto& path: get_files_in_dir("/users/flo/projects/thesis/data/bin")) {
        sample<31> s = deserialize<sample<31>>(path);
        std::vector<byte> data = s.data();
        uint64_t* kmers = reinterpret_cast<uint64_t*>(data.data());
        for (size_t i = 0; i < s.size(); i++) {
            uint64_t kmer = kmers[i];
            frequencies[kmer]++;
        }
        num_kmers += s.size();
        i++;
        std::cout << i << std::endl << num_kmers << std::endl << frequencies.size() << std::endl << std::endl;
        if (frequencies.size() > 90000000) {
//            write_frequencies(frequencies, "/users/flo/projects/thesis/data/freq_" + std::to_string(i) + ".f");
            frequencies.clear();
        }
    }
//    write_frequencies(frequencies, "/users/flo/projects/thesis/data/freq_" + std::to_string(i) + ".f");
}
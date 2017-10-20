#pragma once

#include <string>
#include <vector>
#include <timer.hpp>
#include <file/util.hpp>
#include <numeric>

namespace genome::server {
    template<class T>
    class base {
    protected:
        T m_header;
        stream_metadata m_smd;
        static const uint64_t m_count_expansions[16];
        timer m_timer;

        void create_hashes(std::vector<size_t>& hashes, const std::string& query, uint32_t kmer_size, uint64_t signature_size, uint64_t num_hashes) const {
            hashes.reserve(num_hashes * (query.size() - kmer_size + 1));
            hashes.clear();
            std::vector<char> kmer_data(kmer<31>::data_size(kmer_size));
            for (size_t i = 0; i <= query.size() - kmer_size; i++) {
                kmer<31>::init(query.data() + i, kmer_data.data(), kmer_size);
                genome::bss::create_hashes(kmer_data.data(), kmer<31>::data_size(kmer_size), signature_size, num_hashes,
                                           [&](size_t hash) {
                                               hashes.push_back(hash);
                                           });
            }
        }

        void counts_to_result(const std::vector<std::string>& file_names, const std::vector<uint16_t>& counts,
                              std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) const {
            std::vector<uint32_t> sorted_indices(file_names.size());
            std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
            std::nth_element(sorted_indices.begin(), sorted_indices.begin() + num_results, sorted_indices.end(), [&](const auto v1, const auto v2){
                return counts[v1] > counts[v2];
            });

            result.resize(num_results);
            for (size_t i = 0; i < num_results; i++) {
                result.emplace_back(std::make_pair(counts[sorted_indices[i]], file_names[sorted_indices[i]]));
            }
        }

        explicit base(const boost::filesystem::path& path) {
            std::ifstream ifs;
            m_header = file::deserialize_header<T>(ifs, path);
            m_smd = get_stream_metadata(ifs);
        }
    public:
        virtual void search(const std::string& query, uint32_t kmer_size, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results = 0) = 0;
        virtual ~base() = default;

        const timer& get_timer() const {
            return m_timer;
        }
    };

    template<class T>
    const uint64_t base<T>::m_count_expansions[] = {0, 1, 65536, 65537, 4294967296, 4294967297, 4295032832, 4295032833, 281474976710656,
                                                    281474976710657, 281474976776192, 281474976776193, 281479271677952, 281479271677953,
                                                    281479271743488, 281479271743489};
}


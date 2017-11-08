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
            std::vector<uint32_t> sorted_indices(counts.size()); //todo ??? size here

            std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
            std::partial_sort(sorted_indices.begin(), sorted_indices.begin() + num_results, sorted_indices.end(), [&](auto v1, auto v2){
                return counts[v1] > counts[v2];
            });

            result.reserve(num_results);
            result.clear();
            for (size_t i = 0; i < num_results; i++) {
                result.emplace_back(std::make_pair(counts[sorted_indices[i]], file_names[sorted_indices[i]]));
            }
        }

        void compute_counts(size_t hashes_size, std::vector<uint16_t>& counts, const char* rows) {
            auto rows_b = reinterpret_cast<const byte*>(rows);
            #pragma omp declare reduction (merge : std::vector<uint16_t> : \
                            std::transform(omp_out.begin(), omp_out.end(), omp_in.begin(), omp_out.begin(), std::plus<>())) \
                            initializer(omp_priv = omp_orig)

            //todo pull virtual methods out
            #pragma omp parallel for reduction(merge: counts)
            for (uint64_t i = 0; i < hashes_size; i += num_hashes()) {
               //todo i+= num_hahses() * blocksize ...
                auto counts_64 = reinterpret_cast<uint64_t*>(counts.data());
                auto rows_8 = rows_b + i * block_size();
                //todo each thread should only use k % thread_id ==0
                for (size_t k = 0; k < block_size(); k++) {
                    counts_64[2 * k] += m_count_expansions[rows_8[k] & 0xF];
                    counts_64[2 * k + 1] += m_count_expansions[rows_8[k] >> 4];
                }
            }
        }

        void aggregate_rows(size_t hashes_size, char* rows) {
            #pragma omp parallel for
            for (uint64_t i = 0; i < hashes_size; i += num_hashes()) {
                auto rows_8 = rows + i * block_size();
                auto rows_64 = reinterpret_cast<uint64_t*>(rows_8);
                for (size_t j = 1; j < num_hashes(); j++) {
                    auto rows_8_j = rows_8 + j * block_size();
                    auto rows_64_j = reinterpret_cast<uint64_t*>(rows_8_j);
                    size_t k = 0;
                    while ((k + 1) * 8 <= block_size()) {
                        rows_64[k] &= rows_64_j[k];
                        k++;
                    }
                    k = k * 8;
                    while (k < block_size()) {
                        rows_8[k] &= rows_8_j[k];
                        k++;
                    }
                }
            }
        }

        explicit base(const std::experimental::filesystem::path& path) {
            std::ifstream ifs;
            m_header = file::deserialize_header<T>(ifs, path);
            m_smd = get_stream_metadata(ifs);
        }

        virtual void calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) = 0;
        virtual uint64_t block_size() const = 0;
        virtual uint64_t num_hashes() const = 0;
        virtual uint64_t max_hash_value() const = 0;
        virtual uint64_t counts_size() const = 0;

    public:
        virtual ~base() = default;

        const timer& get_timer() const {
            return m_timer;
        }

        void search(const std::string& query, uint32_t kmer_size, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results = 0) {
            assert(query.size() >= kmer_size);
            assert(query.size() - kmer_size + 1 <= UINT16_MAX);
            m_timer.active("hashes");
            std::vector<size_t> hashes;
            create_hashes(hashes, query, kmer_size, max_hash_value(), num_hashes());
            std::vector<uint16_t> counts(counts_size());
            calculate_counts(hashes, counts);
            m_timer.active("counts_to_result");
            counts_to_result(m_header.file_names(), counts, result, num_results == 0 ? m_header.file_names().size() : num_results);
            m_timer.stop();
        }

    };

    template<class T>
    const uint64_t base<T>::m_count_expansions[] = {0, 1, 65536, 65537, 4294967296, 4294967297, 4295032832, 4295032833, 281474976710656,
                                                    281474976710657, 281474976776192, 281474976776193, 281479271677952, 281479271677953,
                                                    281479271743488, 281479271743489};
}


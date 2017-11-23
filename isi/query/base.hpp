#pragma once

#include <string>
#include <vector>
#include <isi/timer.hpp>
#include <isi/util/file.hpp>
#include <numeric>
#include <immintrin.h>
#include <xxhash.h>
#include <algorithm>

namespace isi::query {
    template<class T>
    class base {
    protected:
        T m_header;
        stream_metadata m_smd;
        static const uint64_t m_count_expansions[16];
        static const uint16_t m_expansion[2048];
        timer m_timer;

        void compute_counts(size_t hashes_size, std::vector<uint16_t>& counts, const char* rows);
        void aggregate_rows(size_t hashes_size, char* rows);
        explicit base(const std::experimental::filesystem::path& path);

        virtual void calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) = 0;
        virtual uint64_t block_size() const = 0;
        virtual uint64_t num_hashes() const = 0;
        virtual uint64_t max_hash_value() const = 0;
        virtual uint64_t counts_size() const = 0;

    public:
        virtual ~base() = default;
        const timer& get_timer() const;
        void search(const std::string& query, uint32_t kmer_size, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results = 0);
    };
}


namespace isi::query {
    namespace {
        void create_hashes(std::vector<size_t>& hashes, const std::string& query, uint32_t kmer_size, uint64_t signature_size, uint64_t num_hashes) {
            size_t kmer_data_size = kmer<31>::data_size(kmer_size);
            size_t num_kmers = query.size() - kmer_size + 1;
            hashes.resize(num_hashes * num_kmers);
            #pragma omp parallel
            {
                std::vector<char> kmer_data(kmer_data_size);
                char* kmer_data_8 = kmer_data.data();
                #pragma omp for
                for (size_t i = 0; i < num_kmers; i++) {
                    kmer<31>::init(query.data() + i, kmer_data_8, kmer_size);
                    for (unsigned int j = 0; j < num_hashes; j++) {
                        size_t hash = XXH32(kmer_data_8, kmer_data_size, j);
                        hashes[i * num_hashes + j] = hash % signature_size;
                    }
                }
            }
        }

        void counts_to_result(const std::vector<std::string>& file_names, const std::vector<uint16_t>& counts,
                              std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) {
            std::vector<uint32_t> sorted_indices(file_names.size());

            std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
            std::partial_sort(sorted_indices.begin(), sorted_indices.begin() + num_results, sorted_indices.end(),
                              [&](auto v1, auto v2) {
                                  return counts[v1] > counts[v2];
                              });

            result.resize(num_results);
            #pragma omp parallel for
            for (size_t i = 0; i < num_results; i++) {
                result[i] = std::make_pair(counts[sorted_indices[i]], file_names[sorted_indices[i]]);
            }
        }
    }

    template <class T>
    void base<T>::compute_counts(size_t hashes_size, std::vector<uint16_t>& counts, const char* rows) {
        auto m_expansion_128 = reinterpret_cast<const __m128i_u*>(m_expansion);
        auto rows_b = reinterpret_cast<const uint8_t*>(rows);
        uint64_t nh = num_hashes();
        uint64_t bs = block_size();

        #pragma omp declare reduction (merge : std::vector<uint16_t> : \
                        std::transform(omp_out.begin(), omp_out.end(), omp_in.begin(), omp_out.begin(), std::plus<uint16_t>())) \
                        initializer(omp_priv = omp_orig)
        #pragma omp parallel reduction(merge: counts)
        {
//        auto counts_64 = reinterpret_cast<uint64_t*>(counts.data());
            auto counts_128 = reinterpret_cast<__m128i_u*>(counts.data());
            #pragma omp for
            for (uint64_t i = 0; i < hashes_size; i += nh) {
                auto rows_8 = rows_b + i * bs;
                for (size_t k = 0; k < bs; k++) {
//                counts_64[2 * k] += m_count_expansions[rows_8[k] & 0xF];
//                counts_64[2 * k + 1] += m_count_expansions[rows_8[k] >> 4];
                    counts_128[k] = _mm_add_epi16(counts_128[k], m_expansion_128[rows_8[k]]);
                }
            }
        }
    }

    template<class T>
    void base<T>::aggregate_rows(size_t hashes_size, char* rows) {
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

    template<class T>
    base<T>::base(const std::experimental::filesystem::path& path) {
        std::ifstream ifs;
        m_header = file::deserialize_header<T>(ifs, path);
        m_smd = get_stream_metadata(ifs);
    }

    template<class T>
    const timer& base<T>::get_timer() const {
        return m_timer;
    }

    template<class T>
    void base<T>::search(const std::string& query, uint32_t kmer_size, std::vector<std::pair<uint16_t, std::string>>& result, size_t num_results) {
        assert_exit(query.size() >= kmer_size,
                    "search query to short, needs to be at least " + std::to_string(kmer_size) + " characters long");
        assert_exit(query.size() - kmer_size < UINT16_MAX,
                    "search query to long, can not be longer than " + std::to_string(UINT16_MAX + kmer_size - 1) + " characters");
        m_timer.active("hashes");
        std::vector<size_t> hashes;
        create_hashes(hashes, query, kmer_size, max_hash_value(), num_hashes());
        std::vector<uint16_t> counts(counts_size());
        calculate_counts(hashes, counts);
        m_timer.active("counts_to_result");
        counts_to_result(m_header.file_names(), counts, result, num_results == 0 ? m_header.file_names().size() : num_results);
        m_timer.stop();
    }

    template<class T>
    const uint64_t base<T>::m_count_expansions[] = {0, 1, 65536, 65537, 4294967296, 4294967297, 4295032832, 4295032833, 281474976710656,
                                                    281474976710657, 281474976776192, 281474976776193, 281479271677952, 281479271677953,
                                                    281479271743488, 281479271743489};


    template<class T>
    const uint16_t base<T>::m_expansion[] alignas(16) = {
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 0, 0, 0, 0, 0, 0,
        1, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 1, 0, 0, 0, 0, 0,
        1, 0, 1, 0, 0, 0, 0, 0,
        0, 1, 1, 0, 0, 0, 0, 0,
        1, 1, 1, 0, 0, 0, 0, 0,
        0, 0, 0, 1, 0, 0, 0, 0,
        1, 0, 0, 1, 0, 0, 0, 0,
        0, 1, 0, 1, 0, 0, 0, 0,
        1, 1, 0, 1, 0, 0, 0, 0,
        0, 0, 1, 1, 0, 0, 0, 0,
        1, 0, 1, 1, 0, 0, 0, 0,
        0, 1, 1, 1, 0, 0, 0, 0,
        1, 1, 1, 1, 0, 0, 0, 0,
        0, 0, 0, 0, 1, 0, 0, 0,
        1, 0, 0, 0, 1, 0, 0, 0,
        0, 1, 0, 0, 1, 0, 0, 0,
        1, 1, 0, 0, 1, 0, 0, 0,
        0, 0, 1, 0, 1, 0, 0, 0,
        1, 0, 1, 0, 1, 0, 0, 0,
        0, 1, 1, 0, 1, 0, 0, 0,
        1, 1, 1, 0, 1, 0, 0, 0,
        0, 0, 0, 1, 1, 0, 0, 0,
        1, 0, 0, 1, 1, 0, 0, 0,
        0, 1, 0, 1, 1, 0, 0, 0,
        1, 1, 0, 1, 1, 0, 0, 0,
        0, 0, 1, 1, 1, 0, 0, 0,
        1, 0, 1, 1, 1, 0, 0, 0,
        0, 1, 1, 1, 1, 0, 0, 0,
        1, 1, 1, 1, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 0, 0,
        1, 0, 0, 0, 0, 1, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0,
        1, 1, 0, 0, 0, 1, 0, 0,
        0, 0, 1, 0, 0, 1, 0, 0,
        1, 0, 1, 0, 0, 1, 0, 0,
        0, 1, 1, 0, 0, 1, 0, 0,
        1, 1, 1, 0, 0, 1, 0, 0,
        0, 0, 0, 1, 0, 1, 0, 0,
        1, 0, 0, 1, 0, 1, 0, 0,
        0, 1, 0, 1, 0, 1, 0, 0,
        1, 1, 0, 1, 0, 1, 0, 0,
        0, 0, 1, 1, 0, 1, 0, 0,
        1, 0, 1, 1, 0, 1, 0, 0,
        0, 1, 1, 1, 0, 1, 0, 0,
        1, 1, 1, 1, 0, 1, 0, 0,
        0, 0, 0, 0, 1, 1, 0, 0,
        1, 0, 0, 0, 1, 1, 0, 0,
        0, 1, 0, 0, 1, 1, 0, 0,
        1, 1, 0, 0, 1, 1, 0, 0,
        0, 0, 1, 0, 1, 1, 0, 0,
        1, 0, 1, 0, 1, 1, 0, 0,
        0, 1, 1, 0, 1, 1, 0, 0,
        1, 1, 1, 0, 1, 1, 0, 0,
        0, 0, 0, 1, 1, 1, 0, 0,
        1, 0, 0, 1, 1, 1, 0, 0,
        0, 1, 0, 1, 1, 1, 0, 0,
        1, 1, 0, 1, 1, 1, 0, 0,
        0, 0, 1, 1, 1, 1, 0, 0,
        1, 0, 1, 1, 1, 1, 0, 0,
        0, 1, 1, 1, 1, 1, 0, 0,
        1, 1, 1, 1, 1, 1, 0, 0,
        0, 0, 0, 0, 0, 0, 1, 0,
        1, 0, 0, 0, 0, 0, 1, 0,
        0, 1, 0, 0, 0, 0, 1, 0,
        1, 1, 0, 0, 0, 0, 1, 0,
        0, 0, 1, 0, 0, 0, 1, 0,
        1, 0, 1, 0, 0, 0, 1, 0,
        0, 1, 1, 0, 0, 0, 1, 0,
        1, 1, 1, 0, 0, 0, 1, 0,
        0, 0, 0, 1, 0, 0, 1, 0,
        1, 0, 0, 1, 0, 0, 1, 0,
        0, 1, 0, 1, 0, 0, 1, 0,
        1, 1, 0, 1, 0, 0, 1, 0,
        0, 0, 1, 1, 0, 0, 1, 0,
        1, 0, 1, 1, 0, 0, 1, 0,
        0, 1, 1, 1, 0, 0, 1, 0,
        1, 1, 1, 1, 0, 0, 1, 0,
        0, 0, 0, 0, 1, 0, 1, 0,
        1, 0, 0, 0, 1, 0, 1, 0,
        0, 1, 0, 0, 1, 0, 1, 0,
        1, 1, 0, 0, 1, 0, 1, 0,
        0, 0, 1, 0, 1, 0, 1, 0,
        1, 0, 1, 0, 1, 0, 1, 0,
        0, 1, 1, 0, 1, 0, 1, 0,
        1, 1, 1, 0, 1, 0, 1, 0,
        0, 0, 0, 1, 1, 0, 1, 0,
        1, 0, 0, 1, 1, 0, 1, 0,
        0, 1, 0, 1, 1, 0, 1, 0,
        1, 1, 0, 1, 1, 0, 1, 0,
        0, 0, 1, 1, 1, 0, 1, 0,
        1, 0, 1, 1, 1, 0, 1, 0,
        0, 1, 1, 1, 1, 0, 1, 0,
        1, 1, 1, 1, 1, 0, 1, 0,
        0, 0, 0, 0, 0, 1, 1, 0,
        1, 0, 0, 0, 0, 1, 1, 0,
        0, 1, 0, 0, 0, 1, 1, 0,
        1, 1, 0, 0, 0, 1, 1, 0,
        0, 0, 1, 0, 0, 1, 1, 0,
        1, 0, 1, 0, 0, 1, 1, 0,
        0, 1, 1, 0, 0, 1, 1, 0,
        1, 1, 1, 0, 0, 1, 1, 0,
        0, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0,
        0, 1, 0, 1, 0, 1, 1, 0,
        1, 1, 0, 1, 0, 1, 1, 0,
        0, 0, 1, 1, 0, 1, 1, 0,
        1, 0, 1, 1, 0, 1, 1, 0,
        0, 1, 1, 1, 0, 1, 1, 0,
        1, 1, 1, 1, 0, 1, 1, 0,
        0, 0, 0, 0, 1, 1, 1, 0,
        1, 0, 0, 0, 1, 1, 1, 0,
        0, 1, 0, 0, 1, 1, 1, 0,
        1, 1, 0, 0, 1, 1, 1, 0,
        0, 0, 1, 0, 1, 1, 1, 0,
        1, 0, 1, 0, 1, 1, 1, 0,
        0, 1, 1, 0, 1, 1, 1, 0,
        1, 1, 1, 0, 1, 1, 1, 0,
        0, 0, 0, 1, 1, 1, 1, 0,
        1, 0, 0, 1, 1, 1, 1, 0,
        0, 1, 0, 1, 1, 1, 1, 0,
        1, 1, 0, 1, 1, 1, 1, 0,
        0, 0, 1, 1, 1, 1, 1, 0,
        1, 0, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 0,
        1, 1, 1, 1, 1, 1, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 1,
        0, 1, 0, 0, 0, 0, 0, 1,
        1, 1, 0, 0, 0, 0, 0, 1,
        0, 0, 1, 0, 0, 0, 0, 1,
        1, 0, 1, 0, 0, 0, 0, 1,
        0, 1, 1, 0, 0, 0, 0, 1,
        1, 1, 1, 0, 0, 0, 0, 1,
        0, 0, 0, 1, 0, 0, 0, 1,
        1, 0, 0, 1, 0, 0, 0, 1,
        0, 1, 0, 1, 0, 0, 0, 1,
        1, 1, 0, 1, 0, 0, 0, 1,
        0, 0, 1, 1, 0, 0, 0, 1,
        1, 0, 1, 1, 0, 0, 0, 1,
        0, 1, 1, 1, 0, 0, 0, 1,
        1, 1, 1, 1, 0, 0, 0, 1,
        0, 0, 0, 0, 1, 0, 0, 1,
        1, 0, 0, 0, 1, 0, 0, 1,
        0, 1, 0, 0, 1, 0, 0, 1,
        1, 1, 0, 0, 1, 0, 0, 1,
        0, 0, 1, 0, 1, 0, 0, 1,
        1, 0, 1, 0, 1, 0, 0, 1,
        0, 1, 1, 0, 1, 0, 0, 1,
        1, 1, 1, 0, 1, 0, 0, 1,
        0, 0, 0, 1, 1, 0, 0, 1,
        1, 0, 0, 1, 1, 0, 0, 1,
        0, 1, 0, 1, 1, 0, 0, 1,
        1, 1, 0, 1, 1, 0, 0, 1,
        0, 0, 1, 1, 1, 0, 0, 1,
        1, 0, 1, 1, 1, 0, 0, 1,
        0, 1, 1, 1, 1, 0, 0, 1,
        1, 1, 1, 1, 1, 0, 0, 1,
        0, 0, 0, 0, 0, 1, 0, 1,
        1, 0, 0, 0, 0, 1, 0, 1,
        0, 1, 0, 0, 0, 1, 0, 1,
        1, 1, 0, 0, 0, 1, 0, 1,
        0, 0, 1, 0, 0, 1, 0, 1,
        1, 0, 1, 0, 0, 1, 0, 1,
        0, 1, 1, 0, 0, 1, 0, 1,
        1, 1, 1, 0, 0, 1, 0, 1,
        0, 0, 0, 1, 0, 1, 0, 1,
        1, 0, 0, 1, 0, 1, 0, 1,
        0, 1, 0, 1, 0, 1, 0, 1,
        1, 1, 0, 1, 0, 1, 0, 1,
        0, 0, 1, 1, 0, 1, 0, 1,
        1, 0, 1, 1, 0, 1, 0, 1,
        0, 1, 1, 1, 0, 1, 0, 1,
        1, 1, 1, 1, 0, 1, 0, 1,
        0, 0, 0, 0, 1, 1, 0, 1,
        1, 0, 0, 0, 1, 1, 0, 1,
        0, 1, 0, 0, 1, 1, 0, 1,
        1, 1, 0, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        1, 0, 1, 0, 1, 1, 0, 1,
        0, 1, 1, 0, 1, 1, 0, 1,
        1, 1, 1, 0, 1, 1, 0, 1,
        0, 0, 0, 1, 1, 1, 0, 1,
        1, 0, 0, 1, 1, 1, 0, 1,
        0, 1, 0, 1, 1, 1, 0, 1,
        1, 1, 0, 1, 1, 1, 0, 1,
        0, 0, 1, 1, 1, 1, 0, 1,
        1, 0, 1, 1, 1, 1, 0, 1,
        0, 1, 1, 1, 1, 1, 0, 1,
        1, 1, 1, 1, 1, 1, 0, 1,
        0, 0, 0, 0, 0, 0, 1, 1,
        1, 0, 0, 0, 0, 0, 1, 1,
        0, 1, 0, 0, 0, 0, 1, 1,
        1, 1, 0, 0, 0, 0, 1, 1,
        0, 0, 1, 0, 0, 0, 1, 1,
        1, 0, 1, 0, 0, 0, 1, 1,
        0, 1, 1, 0, 0, 0, 1, 1,
        1, 1, 1, 0, 0, 0, 1, 1,
        0, 0, 0, 1, 0, 0, 1, 1,
        1, 0, 0, 1, 0, 0, 1, 1,
        0, 1, 0, 1, 0, 0, 1, 1,
        1, 1, 0, 1, 0, 0, 1, 1,
        0, 0, 1, 1, 0, 0, 1, 1,
        1, 0, 1, 1, 0, 0, 1, 1,
        0, 1, 1, 1, 0, 0, 1, 1,
        1, 1, 1, 1, 0, 0, 1, 1,
        0, 0, 0, 0, 1, 0, 1, 1,
        1, 0, 0, 0, 1, 0, 1, 1,
        0, 1, 0, 0, 1, 0, 1, 1,
        1, 1, 0, 0, 1, 0, 1, 1,
        0, 0, 1, 0, 1, 0, 1, 1,
        1, 0, 1, 0, 1, 0, 1, 1,
        0, 1, 1, 0, 1, 0, 1, 1,
        1, 1, 1, 0, 1, 0, 1, 1,
        0, 0, 0, 1, 1, 0, 1, 1,
        1, 0, 0, 1, 1, 0, 1, 1,
        0, 1, 0, 1, 1, 0, 1, 1,
        1, 1, 0, 1, 1, 0, 1, 1,
        0, 0, 1, 1, 1, 0, 1, 1,
        1, 0, 1, 1, 1, 0, 1, 1,
        0, 1, 1, 1, 1, 0, 1, 1,
        1, 1, 1, 1, 1, 0, 1, 1,
        0, 0, 0, 0, 0, 1, 1, 1,
        1, 0, 0, 0, 0, 1, 1, 1,
        0, 1, 0, 0, 0, 1, 1, 1,
        1, 1, 0, 0, 0, 1, 1, 1,
        0, 0, 1, 0, 0, 1, 1, 1,
        1, 0, 1, 0, 0, 1, 1, 1,
        0, 1, 1, 0, 0, 1, 1, 1,
        1, 1, 1, 0, 0, 1, 1, 1,
        0, 0, 0, 1, 0, 1, 1, 1,
        1, 0, 0, 1, 0, 1, 1, 1,
        0, 1, 0, 1, 0, 1, 1, 1,
        1, 1, 0, 1, 0, 1, 1, 1,
        0, 0, 1, 1, 0, 1, 1, 1,
        1, 0, 1, 1, 0, 1, 1, 1,
        0, 1, 1, 1, 0, 1, 1, 1,
        1, 1, 1, 1, 0, 1, 1, 1,
        0, 0, 0, 0, 1, 1, 1, 1,
        1, 0, 0, 0, 1, 1, 1, 1,
        0, 1, 0, 0, 1, 1, 1, 1,
        1, 1, 0, 0, 1, 1, 1, 1,
        0, 0, 1, 0, 1, 1, 1, 1,
        1, 0, 1, 0, 1, 1, 1, 1,
        0, 1, 1, 0, 1, 1, 1, 1,
        1, 1, 1, 0, 1, 1, 1, 1,
        0, 0, 0, 1, 1, 1, 1, 1,
        1, 0, 0, 1, 1, 1, 1, 1,
        0, 1, 0, 1, 1, 1, 1, 1,
        1, 1, 0, 1, 1, 1, 1, 1,
        0, 0, 1, 1, 1, 1, 1, 1,
        1, 0, 1, 1, 1, 1, 1, 1,
        0, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1};
}

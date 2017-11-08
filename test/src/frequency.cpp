#include <gtest/gtest.h>
#include <iostream>
#include <experimental/filesystem>
#include <fstream>
#include <frequency.hpp>
#include "util.hpp"

namespace {

    std::string in_dir = "test/resources/frequency/input/";
    std::string out_dir = "test/out/frequency/";
    std::string result_bin = "test/resources/frequency/result/bin.g_freq";
    std::string result_freq = "test/resources/frequency/result/freq.g_freq";
    std::string sample_1 = in_dir + "sample_1.g_sam";
    std::string sample_2 = in_dir + "sample_2.g_sam";
    std::string sample_3 = in_dir + "sample_3.g_sam";

    /*
    std::string sample_4 = in_dir + "sample_4.g_freq";
    std::string sample_5 = in_dir + "sample_5.g_freq";
    void generate_result_bin() {
        std::experimental::filesystem::create_directories(out_dir);
        std::vector<uint64_t> v;
        sample<31> s;
        file::deserialize(sample_1, s);
        v.insert(v.end(), reinterpret_cast<uint64_t*>(&(*s.data().begin())), reinterpret_cast<uint64_t*>(&(*s.data().end())));
        file::deserialize(sample_2, s);
        v.insert(v.end(), reinterpret_cast<uint64_t*>(&(*s.data().begin())), reinterpret_cast<uint64_t*>(&(*s.data().end())));
        file::deserialize(sample_3, s);
        v.insert(v.end(), reinterpret_cast<uint64_t*>(&(*s.data().begin())), reinterpret_cast<uint64_t*>(&(*s.data().end())));
        std::sort(v.begin(), v.end());

        std::ofstream ofs;
        file::serialize_header<file::frequency_header>(ofs, "bin" + file::frequency_header::file_extension, file::frequency_header());
        uint64_t kmer = v[0];
        uint32_t count = 1;
        for(size_t i = 1; i < v.size(); i++) {
            if (kmer == v[i]) {
                count++;
            } else {
                ofs.write(reinterpret_cast<const char*>(&kmer), 8);
                ofs.write(reinterpret_cast<const char*>(&count), 4);
                kmer = v[i];
                count = 1;
            }
        }
        ofs.write(reinterpret_cast<const char*>(&kmer), 8);
        ofs.write(reinterpret_cast<const char*>(&count), 4);
    }

    template<class OutputIteratorKmer, class OutputIteratorCount>
    void read_sample(const std::string& file, OutputIteratorKmer& iter_kmer, OutputIteratorCount& iter_count) {
        std::ifstream ifs;
        file::deserialize_header<file::frequency_header>(ifs, file);
        while(ifs && ifs.peek() != EOF) {
            uint64_t kmer;
            uint32_t count;
            ifs.read(reinterpret_cast<char*>(&kmer), 8);
            ifs.read(reinterpret_cast<char*>(&count), 4);
            *iter_kmer++ = kmer;
            *iter_count++ = count;
        }
    }

    void generate_result_fre() {
        std::experimental::filesystem::create_directories(out_dir);
        std::vector<uint64_t> kmers;
        std::vector<uint32_t> counts;
        std::back_insert_iterator<std::vector<uint64_t>> iter_kmer(kmers);
        std::back_insert_iterator<std::vector<uint32_t >> iter_count(counts);
        read_sample(sample_4, iter_kmer, iter_count);
        read_sample(sample_5, iter_kmer, iter_count);
        std::vector<size_t> indices(kmers.size());
        for (size_t i = 0; i < kmers.size(); i++) {
            indices[i] = i;
        }
        std::sort(indices.begin(), indices.end(), [&](const auto& i1, const auto& i2) {
            return kmers[i1] < kmers[i2];
        });

        std::ofstream ofs;
        file::serialize_header<file::frequency_header>(ofs, "freq" + file::frequency_header::file_extension, file::frequency_header());
        uint64_t kmer = kmers[indices[0]];
        uint32_t count = counts[indices[0]];
        for(size_t i = 1; i < kmers.size(); i++) {
            if (kmer == kmers[indices[i]]) {
                count += counts[indices[i]];
            } else {
                ofs.write(reinterpret_cast<const char*>(&kmer), 8);
                ofs.write(reinterpret_cast<const char*>(&count), 4);
                kmer = kmers[indices[i]];
                count = counts[indices[i]];
            }
        }
        ofs.write(reinterpret_cast<const char*>(&kmer), 8);
        ofs.write(reinterpret_cast<const char*>(&count), 4);
    }

    TEST(frequency, generate_results) {
        generate_result_bin();
        generate_result_freq();
    }
    */


    size_t get_count_sample(const std::string& file) {
        std::ifstream ifs;
        genome::file::deserialize_header<genome::file::sample_header>(ifs, file);
        size_t total_count = 0;
        uint64_t kmer;
        while (ifs && ifs.peek() != EOF) {
            ifs.read(reinterpret_cast<char*>(&kmer), 8);
            total_count++;
        }
        return total_count;
    }

    size_t get_count_frequency(const std::string& file) {
        std::ifstream ifs;
        genome::file::deserialize_header<genome::file::frequency_header>(ifs, file);
        size_t total_count = 0;
        uint64_t kmer;
        uint32_t count;
        while (ifs && ifs.peek() != EOF) {
            ifs.read(reinterpret_cast<char*>(&kmer), 8);
            ifs.read(reinterpret_cast<char*>(&count), 4);
            total_count += count;
        }
        return total_count;
    }

    class frequency : public ::testing::Test {
    protected:
        virtual void SetUp() {

            std::error_code ec;
            std::experimental::filesystem::remove_all(out_dir, ec);
        }
    };

    TEST_F(frequency, bin) {
        genome::frequency::process_all_in_directory<genome::frequency::bin_pq_element>(in_dir, out_dir, 40);
        size_t total_count = get_count_sample(sample_1) + get_count_sample(sample_2) + get_count_sample(sample_3);
        ASSERT_EQ(get_count_frequency(out_dir + "[sample_1-sample_3]" + genome::file::frequency_header::file_extension), total_count);
        assert_equals_files(result_bin, out_dir + "[sample_1-sample_3]" + genome::file::frequency_header::file_extension);
    }

    TEST_F(frequency, freq) {
        genome::frequency::process_all_in_directory<genome::frequency::fre_pq_element>(in_dir, out_dir, 40);
        assert_equals_files(result_freq, out_dir + "[sample_4-sample_5]" + genome::file::frequency_header::file_extension);
    }
}



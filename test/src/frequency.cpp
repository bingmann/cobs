#include <gtest/gtest.h>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <fstream>
#include <frequency.hpp>

namespace {

    std::string in_dir = "test/resources/frequency/input/";
    std::string out_dir = "test/out/frequency/";
    std::string result_bin = "test/resources/frequency/result/bin.f";
    std::string result_freq = "test/resources/frequency/result/freq.f";
    std::string sample_1 = in_dir + "sample_1.b";
    std::string sample_2 = in_dir + "sample_2.b";
    std::string sample_3 = in_dir + "sample_3.b";
    std::string sample_4 = in_dir + "sample_4.f";
    std::string sample_5 = in_dir + "sample_5.f";

    template<class OutputIterator>
    void read_sample(const std::string& file, OutputIterator& iter) {
        std::ifstream ifs(file, std::ios::in | std::ios::binary);
        while(ifs && ifs.peek() != EOF) {
            uint64_t kmer;
            ifs.read(reinterpret_cast<char*>(&kmer), 8);
            *iter++ = kmer;
        }
    }

    void generate_result_bin() {
        boost::filesystem::create_directories(out_dir);
        std::vector<uint64_t> v;
        std::back_insert_iterator<std::vector<uint64_t>> iter(v);
        read_sample(sample_1, iter);
        read_sample(sample_2, iter);
        read_sample(sample_3, iter);
        std::sort(v.begin(), v.end());

        std::ofstream ofs("bin.f", std::ios::out | std::ios::binary);
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
        std::ifstream ifs(file, std::ios::in | std::ios::binary);
        while(ifs && ifs.peek() != EOF) {
            uint64_t kmer;
            uint32_t count;
            ifs.read(reinterpret_cast<char*>(&kmer), 8);
            ifs.read(reinterpret_cast<char*>(&count), 4);
            *iter_kmer++ = kmer;
            *iter_count++ = count;
        }
    }

    void generate_result_freq() {
        boost::filesystem::create_directories(out_dir);
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

        std::ofstream ofs("freq.f", std::ios::out | std::ios::binary);
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

    void assert_equals_files(const std::string& f1, const std::string& f2) {
        std::ifstream ifs1(f1, std::ios::in | std::ios::binary);
        std::ifstream ifs2(f2, std::ios::in | std::ios::binary);
        std::istream_iterator<char> start1(ifs1);
        std::istream_iterator<char> start2(ifs2);
        std::istream_iterator<char> end;
        std::vector<char> v1(start1, end);
        std::vector<char> v2(start2, end);

        ASSERT_EQ(v1.size(), v2.size());
        for (size_t i = 0; i < v1.size(); i++) {
            ASSERT_EQ(v1[i], v2[i]);
        }
    }

    size_t get_count(const std::string& file) {
        std::ifstream ifs(file, std::ios::in | std::ios::binary);
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

    TEST(frequency, generate_results) {
//        generate_result_bin();
//        generate_result_freq();
    }

    TEST(frequency, bin) {
        boost::filesystem::remove_all(out_dir);
        frequency::process_all_in_directory<frequency::bin_pq_element>(in_dir, out_dir);
        size_t total_count = (boost::filesystem::file_size(sample_1) + boost::filesystem::file_size(sample_2) + boost::filesystem::file_size(sample_3)) / 8;
        ASSERT_EQ(get_count(out_dir + "1.f"), total_count);
        assert_equals_files(result_bin, out_dir + "1.f");
    }

    TEST(frequency, freq) {
        boost::filesystem::remove_all(out_dir);
        frequency::process_all_in_directory<frequency::freq_pq_element>(in_dir, out_dir);
        assert_equals_files(result_freq, out_dir + "1.f");
    }
}



#include <gtest/gtest.h>
#include <server/abss/mmap.hpp>
#include <bit_sliced_signatures/abss.hpp>

namespace {
    std::string in_dir = "test/out/server_abss/input_1/";
    std::string abss_path = in_dir + "filter.g_abss";
    std::string tmp_dir = "test/out/server_abss/tmp";
    std::string query = "CAATCGAGCAAGTCCATTATCAACGAGTGTGTTGCAGTTTTATTCTCTCGCCAGCATTGTAATAGGCACTAAAAGAGTGATGATAGTCATGAGTGCTGAGCTAAGA"
            "CGGCGTCGGTGCATAGCGGACTTTCGGTCAGTCGCAATTCCTCACGAGACCCGTCCTGTTGAGCGTATCACTCTCAATGTACAAGCAACCCGAGAAGGCTGTGCCT"
            "GGACTCAACCGGATGCAGGATGGACTCCAGACACGGGGCCACCACTCTTCACACGTAAAGCAAGAACGTCGAGCAGTCATGAAAGTCTTAGTACCGCACGTGCCAT"
            "CTTACTGCGAATATTGCCTGAAGCTGTACCGTTATTGGGGGGCAAAGATGAAGTTCTCCTCTTTTCATAATTGTACTGACGACAGCCGTGTTCCCGGTTTCTTCAG"
            "AGGTTAAAGAATAAGGGCTTATTGTAGGCAGAGGGACGCCCTTTTAGTGGCTGGCGTTAAGTATCTTCGGACCCCCTTGTCTATCCAGATTAATCGAATTCTCTCA"
            "TTTAGGACCCTAGTAAGTCATCATTGGTATTTGAATGCGACCCCGAAGAAACCGCCTAAAAATGTCAATGGTTGGTCCACTAAACTTCATTTAATCAACTCCTAAA"
            "TCGGCGCGATAGGCCATTAGAGGTTTAATTTTGTATGGCAAGGTACTTCCGATCTTAATGAATGGCCGGAAGAGGTACGGACGCGATATGCGGGGGTGAGAGGGCA"
            "AATAGGCAGGTTCGCCTTCGTCACGCTAGGAGGCAATTCTATAAGAATGCACATTGCATCGATACATAAAATGTCTCGACCGCATGCGCAACTTGTGAAGTGTCTA"
            "CTATCCCTAAGCCCATTTCCCGCATAATAACCCCTGATTGTGTCCGCATCTGATGCTACCCGGGTTGAGTTAGCGTCGAGCTCGCGGAACTTATTGCATGAGTAGA"
            "GTTGAGTAAGAGCTGTTAGATGGCTCGCTGAGCTAATAGTTGCCCA";
    std::string query2 = "TTCGCACTGTCGGGGTCCCTTGGGTGTTTTGCACTAGCGTCAGGTAGGCTAGTATGTGTCTTTCCTTCCAGGGGTATGTGGCTGCGTGGTCAAATGTGCAGCATAC"
            "GTATTTGCTCGGCGTGCTTGGTCTCTCGTACTTCTCCTGGAGATCAAGGAAATGTTTCTTGTCCAAGCGGACAGCGGTTCTACGGAATGGATCTACGTTACTGCCTG"
            "CATAAGGAGAACGGAGTTGCCAAGGACGAAAGCGACTCTAGGTTCTAACCGTCGACTTTGGCGGAAAGGTTTCACTCAGGAAGCAGACACTGATTGACACGGTTTAG"
            "CAGAACGTTTGAGGATTAGGTCAAATTGAGTGGTTTAATATCGGTATGTCTGGGATTAAAATATAGTATAGTGCGCTGATCGGAGACGAATTAAAGACACGAGTTCC"
            "CAAAACCAGGCGGGCTCGCCACGACGGCTAATCCTGGTAGTTTACGTGAACAATGTTCTGAAGAAAATTTATGAAAGAAGGACCCGTCATCGCCTACAATTACCTAC"
            "AACGGTCGACCATACCTTCGATTGTCGTGGCCACCCTCGGATTACACGGCAGAGGTGGTTGTGTTCCGATAGGCCAGCATATTATCCTAAGGCGTTACCCCAATCGT"
            "TTTCCGTCGGATTTGCTATAGCCCCTGAACGCTACATGCACGAAACCAAGTTATGTATGCACTGGGTCATCAATAGGACATAGCCTTGTAGTTAACATGTAGCCCGG"
            "CCGTATTAGTACAGTAGAGCCTTCACCGGCATTCTGTTTATTAAGTTATTTCTACAGCAAAACGATCATATGCAGATCCGCAGTGCGCGGTAGAGACACGTCCACCC"
            "GGCTGCTCTGTAATAGGGACTAAAAAAGTGATGATTATCATGAGTGCCCCGTTATGGTCGTGTTCGATCAGAGCGCTCTTACGAGCAGTCGTATGCTTTCTCGAATT"
            "CCGTGCGGTTAAGCGTGACAGTCCCAGTGAACCCACAA";

    std::vector<isi::sample<31>> generate_samples_all() {
        std::vector<isi::sample<31>> samples(33);
        isi::kmer<31> k;
        for (size_t i = 0; i < query.size() - 31; i++) {
            k.init(query.data() + i);
            for (size_t j = 0; j < samples.size(); j++) {
                if (j % (i % (samples.size() - 1) + 1) == 0) {
                    samples[j].data().push_back(k);
                }
            }
        }
        return samples;
    }

    std::vector<isi::sample<31>> generate_samples_one() {
        std::vector<isi::sample<31>> samples(33);
        isi::kmer<31> k;
        k.init(query.data());
        for (size_t i = 0; i < samples.size(); i++) {
            for (size_t j = 0; j < i * 10 + 1; j++) {
                samples[i].data().push_back(k);
            }
        }
        return samples;
    }

    void generate_test_case(std::vector<isi::sample<31>> samples) {
        for (size_t i = 0; i < samples.size(); i++) {
            std::string index = (i < 10 ? "0" : "") + std::to_string(i);
            isi::file::serialize(tmp_dir + "/sample_" + index + isi::file::sample_header::file_extension, samples[i]);
        }

    }

    class server_abss : public ::testing::Test {
    protected:
        virtual void SetUp() {

            std::error_code ec;
            std::experimental::filesystem::remove_all(in_dir, ec);
            std::experimental::filesystem::remove_all(tmp_dir, ec);
            std::experimental::filesystem::create_directories(in_dir);
            std::experimental::filesystem::create_directories(tmp_dir);
        }
    };

    TEST_F(server_abss, padding) {
        auto samples = generate_samples_all();
        generate_test_case(samples);
        size_t page_size = isi::get_page_size();
        isi::abss::create_folders(tmp_dir, in_dir, page_size);
        isi::abss::create_abss_from_samples(in_dir, 8, 3, 0.1, page_size);
        std::ifstream ifs;
        auto abssh = isi::file::deserialize_header<isi::file::abss_header>(ifs, abss_path);
        isi::stream_metadata smd = isi::get_stream_metadata(ifs);
        ASSERT_EQ(smd.curr_pos % page_size, 0U);
    }

    TEST_F(server_abss, deserialization) {
        auto samples = generate_samples_all();
        generate_test_case(samples);
        isi::abss::create_folders(tmp_dir, in_dir, 2);
        isi::abss::create_abss_from_samples(in_dir, 8, 3, 0.1, 2);
        std::vector<std::vector<isi::byte>> data;
        isi::file::abss_header abssh;
        isi::file::deserialize(abss_path, data, abssh);
        ASSERT_EQ(abssh.file_names().size(), 33U);
        ASSERT_EQ(abssh.parameters().size(), 3U);
        ASSERT_EQ(data.size(), 3U);
    }

    TEST_F(server_abss, all_included) {
        auto samples = generate_samples_all();
        generate_test_case(samples);
        isi::abss::create_folders(tmp_dir, in_dir, 2);
        isi::abss::create_abss_from_samples(in_dir, 8, 3, 0.1, 2);
        isi::server::abss::mmap s_mmap(abss_path);
        std::vector<std::pair<uint16_t, std::string>> result;

        s_mmap.search(query, 31, result);
        std::vector<std::string> split;
        ASSERT_EQ(samples.size(), result.size());
        for(auto& r: result) {
            int index = std::stoi(r.second.substr(r.second.size() - 2));
            ASSERT_GE(r.first, samples[index].data().size());
        }
    }

    TEST_F(server_abss, one_included) {
        auto samples = generate_samples_one();
        generate_test_case(samples);
        isi::abss::create_folders(tmp_dir, in_dir, 2);
        isi::abss::create_abss_from_samples(in_dir, 8, 3, 0.1, 2);
        isi::server::abss::mmap s_mmap(abss_path);
        std::vector<std::pair<uint16_t, std::string>> result;

        s_mmap.search(query, 31, result);
        ASSERT_EQ(samples.size(), result.size());
        for(auto& r: result) {
            ASSERT_EQ(r.first, 1);
        }
    }
}

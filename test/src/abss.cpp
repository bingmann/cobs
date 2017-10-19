#include <gtest/gtest.h>
#include <bit_sliced_signatures/abss.hpp>
#include <iostream>

namespace {
    using namespace genome;

    std::string in_dir = "test/out/abss/input_1/";
    std::string abss_path = in_dir + "filter.g_abss";
    std::string tmp_dir = "test/out/abss/tmp";
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

    void generate_test_case() {
        std::vector<sample<31>> samples(33);
        kmer<31> k;
        for (size_t i = 0; i < query.size() - 31; i++) {
            k.init(query.data() + i);
            for (size_t j = 0; j < samples.size(); j++) {
                if (j % (i % (samples.size() - 1) + 1) == 0) {
                    samples[j].data().push_back(k);
                }
            }
        }

        for (size_t i = 0; i < samples.size(); i++) {
            genome::file::serialize(tmp_dir + "/sample_" + std::to_string(i) + genome::file::sample_header::file_extension, samples[i]);
        }

    }

    class abss : public ::testing::Test {
    protected:
        virtual void SetUp() {
            boost::filesystem::remove_all(in_dir);
            boost::filesystem::remove_all(tmp_dir);
            boost::filesystem::create_directories(in_dir);
            boost::filesystem::create_directories(tmp_dir);
            generate_test_case();
        }
    };

    TEST_F(abss, padding) {
        size_t page_size = get_page_size();
        genome::abss::create_folders(tmp_dir, in_dir, page_size);
        genome::abss::create_abss_from_samples(in_dir, 8, 3, 0.1, page_size);
        std::ifstream ifs;
        auto abssh = genome::file::deserialize_header<genome::file::abss_header>(ifs, abss_path);
        genome::stream_metadata smd = genome::get_stream_metadata(ifs);
        ASSERT_EQ(smd.curr_pos % page_size, 0U);
    }

    TEST_F(abss, deserialization) {
        genome::abss::create_folders(tmp_dir, in_dir, 2);
        genome::abss::create_abss_from_samples(in_dir, 8, 3, 0.1, 2);
        std::vector<std::vector<byte>> data;
        genome::file::abss_header abssh;
        genome::file::deserialize(abss_path, data, abssh);
        ASSERT_EQ(abssh.file_names().size(), 40U);
        ASSERT_EQ(abssh.parameters().size(), 3U);
        ASSERT_EQ(data.size(), 3U);
    }
}

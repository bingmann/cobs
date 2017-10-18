#include <gtest/gtest.h>
#include <msbf.hpp>
#include <iostream>

namespace {
    using namespace genome;

    std::string in_dir = "test/resources/msbf/input_1/";
    std::string msbf_path = in_dir + "filter.g_mbf";
    std::string tmp_dir = "test/out/tmp";
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

    class msbf : public ::testing::Test {
    protected:
        virtual void SetUp() {
            boost::filesystem::create_directories(in_dir);
            boost::filesystem::create_directories(tmp_dir);
            generate_test_case();
            genome::msbf::create_folders(tmp_dir, in_dir, 16);
            genome::msbf::create_msbf_from_samples(in_dir, 8, 3, 0.1);
        }

        virtual void TearDown() {
            boost::filesystem::remove_all(in_dir);
            boost::filesystem::remove_all(tmp_dir);
        }
    };

    TEST_F(msbf, padding) {
        std::ifstream ifs;
        auto msbfh = genome::file::deserialize_header<genome::file::msbf_header>(ifs, msbf_path);
        genome::stream_metadata smd = genome::get_stream_metadata(ifs);
        ASSERT_EQ(smd.curr_pos % getpagesize(), 0U);
    }

    TEST_F(msbf, deserialization) {
        std::vector<std::vector<byte>> data;
        genome::file::msbf_header msbfh;
        genome::file::deserialize(msbf_path, data, msbfh);
        ASSERT_EQ(msbfh.file_names().size(), 40U);
        ASSERT_EQ(data.size(), 3U);
    }
}

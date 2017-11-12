#include <gtest/gtest.h>
#include <server/compact_index/mmap.hpp>
#include <isi/compact_index.hpp>

namespace {
    std::string in_dir = "test/out/server_compact_index/input_1/";
    std::string compact_index_path = in_dir + "filter.g_cisi";
    std::string tmp_dir = "test/out/server_compact_index/tmp";
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

    class server_compact_index : public ::testing::Test {
    protected:
        virtual void SetUp() {

            std::error_code ec;
            std::experimental::filesystem::remove_all(in_dir, ec);
            std::experimental::filesystem::remove_all(tmp_dir, ec);
            std::experimental::filesystem::create_directories(in_dir);
            std::experimental::filesystem::create_directories(tmp_dir);
        }
    };

    TEST_F(server_compact_index, padding) {
        auto samples = generate_samples_all();
        generate_test_case(samples);
        size_t page_size = isi::get_page_size();
        isi::compact_index::create_folders(tmp_dir, in_dir, page_size);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, page_size);
        std::ifstream ifs;
        auto h = isi::file::deserialize_header<isi::file::compact_index_header>(ifs, compact_index_path);
        isi::stream_metadata smd = isi::get_stream_metadata(ifs);
        ASSERT_EQ(smd.curr_pos % page_size, 0U);
    }

    TEST_F(server_compact_index, deserialization) {
        auto samples = generate_samples_all();
        generate_test_case(samples);
        isi::compact_index::create_folders(tmp_dir, in_dir, 2);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, 2);
        std::vector<std::vector<uint8_t>> data;
        isi::file::compact_index_header h;
        isi::file::deserialize(compact_index_path, data, h);
        ASSERT_EQ(h.file_names().size(), 33U);
        ASSERT_EQ(h.parameters().size(), 3U);
        ASSERT_EQ(data.size(), 3U);
    }

    TEST_F(server_compact_index, all_included) {
        auto samples = generate_samples_all();
        generate_test_case(samples);
        isi::compact_index::create_folders(tmp_dir, in_dir, 2);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, 2);
        isi::server::compact_index::mmap s_mmap(compact_index_path);
        std::vector<std::pair<uint16_t, std::string>> result;

        s_mmap.search(query, 31, result);
        std::vector<std::string> split;
        ASSERT_EQ(samples.size(), result.size());
        for(auto& r: result) {
            int index = std::stoi(r.second.substr(r.second.size() - 2));
            ASSERT_GE(r.first, samples[index].data().size());
        }
    }

    TEST_F(server_compact_index, one_included) {
        auto samples = generate_samples_one();
        generate_test_case(samples);
        isi::compact_index::create_folders(tmp_dir, in_dir, 2);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, 2);
        isi::server::compact_index::mmap s_mmap(compact_index_path);
        std::vector<std::pair<uint16_t, std::string>> result;

        s_mmap.search(query, 31, result);
        ASSERT_EQ(samples.size(), result.size());
        for(auto& r: result) {
            ASSERT_EQ(r.first, 1);
        }
    }
}

#include <gtest/gtest.h>
#include "test_util.hpp"

namespace {
    std::experimental::filesystem::path in_dir("test/out/compact_index/input_1");
    std::experimental::filesystem::path samples_dir(in_dir.string() + "/samples");
    std::experimental::filesystem::path bloom_2_dir(in_dir.string() + "/bloom_2");
    std::experimental::filesystem::path compact_index_path(in_dir.string() + "/filter.g_cisi");
    std::experimental::filesystem::path tmp_dir("test/out/compact_index/tmp");

    std::string query = isi::random_sequence(10000, 1);

    class compact_index : public ::testing::Test {
    protected:
        virtual void SetUp() {
            std::error_code ec;
            std::experimental::filesystem::remove_all(in_dir, ec);
            std::experimental::filesystem::remove_all(tmp_dir, ec);
            std::experimental::filesystem::create_directories(in_dir);
            std::experimental::filesystem::create_directories(tmp_dir);
        }
    };

    TEST_F(compact_index, padding) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);
        size_t page_size = isi::get_page_size();
        isi::compact_index::create_folders(tmp_dir, in_dir, page_size);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, page_size);
        std::ifstream ifs;
        isi::file::deserialize_header<isi::file::compact_index_header>(ifs, compact_index_path);
        isi::stream_metadata smd = isi::get_stream_metadata(ifs);
        ASSERT_EQ(smd.curr_pos % page_size, 0U);
    }

    TEST_F(compact_index, deserialization) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);
        isi::compact_index::create_folders(tmp_dir, in_dir, 2);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, 2);
        std::vector<std::vector<uint8_t>> data;
        isi::file::compact_index_header h;
        isi::file::deserialize(compact_index_path, data, h);
        ASSERT_EQ(h.file_names().size(), 33U);
        ASSERT_EQ(h.parameters().size(), 3U);
        ASSERT_EQ(data.size(), 3U);
    }

    TEST_F(compact_index, parameters) {
        uint64_t num_hashes = 3;
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);
        isi::compact_index::create_folders(tmp_dir, in_dir, 2);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, num_hashes, 0.1, 2);
        std::vector<std::vector<uint8_t>> data;
        std::ifstream ifs;
        auto h = isi::file::deserialize_header<isi::file::compact_index_header>(ifs, compact_index_path);

        std::vector<uint64_t> sample_sizes;
        std::vector<isi::file::compact_index_header::parameter> parameters;
        for(const std::experimental::filesystem::path& p: std::experimental::filesystem::recursive_directory_iterator(samples_dir)) {
            if (p.extension() == isi::file::classic_index_header::file_extension) {
                sample_sizes.push_back(std::experimental::filesystem::file_size(p));
            }
        }

        std::sort(sample_sizes.begin(), sample_sizes.end());
        for (size_t i = 0; i < sample_sizes.size(); i++) {
            if (i % 8 == 7) {
                uint64_t signature_size = isi::calc_signature_size(sample_sizes[i] / 8, num_hashes, 0.1);
                ASSERT_EQ(h.parameters()[i / 8].signature_size, signature_size);
                ASSERT_EQ(h.parameters()[i / 8].num_hashes, num_hashes);
            }
        }
    }

    TEST_F(compact_index, num_kmers_calculation) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);
        std::experimental::filesystem::path path_sample(tmp_dir.string() + "/sample_00.g_sam");
        isi::sample<31> s;
        isi::file::deserialize(path_sample, s);

        size_t file_size = std::experimental::filesystem::file_size(path_sample);
        ASSERT_EQ(s.data().size(), file_size / 8 - 2);
    }

    TEST_F(compact_index, num_ones) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);
        isi::compact_index::create_folders(tmp_dir, in_dir, 2);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, 2);
        std::vector<std::vector<uint8_t>> data;
        isi::file::compact_index_header h;
        isi::file::deserialize(compact_index_path, data, h);

        std::vector<std::map<std::string, size_t>> num_ones(h.parameters().size());
        for (size_t i = 0; i < h.parameters().size(); i++) {
            for (size_t j = 0; j < h.parameters()[i].signature_size; j++) {
                for (size_t k = 0; k < h.page_size(); k++) {
                    uint8_t d = data[i][j * h.page_size() + k];
                    for (size_t o = 0; o < 8; o++) {
                        size_t file_names_index = i * h.page_size() * 8 + k * 8 + o;
                        if (file_names_index < h.file_names().size()) {
                            std::string file_name = h.file_names()[file_names_index];
                            num_ones[i][file_name] += (d & (1 << o)) >> o;
                        }
                    }
                }
            }
        }

        for (size_t i = 0; i < h.parameters().size(); i++) {
            double set_bit_ratio = isi::calc_average_set_bit_ratio(h.parameters()[i].signature_size, 3, 0.1);
            double num_ones_average = set_bit_ratio * h.parameters()[i].signature_size;
            for (auto& no: num_ones[i]) {
                ASSERT_LE(no.second, num_ones_average * 1.01);
            }
        }
    }

    TEST_F(compact_index, content) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);
        isi::compact_index::create_folders(tmp_dir, in_dir, 2);
        isi::compact_index::create_compact_index_from_samples(in_dir, 8, 3, 0.1, 2);
        std::vector<std::vector<uint8_t>> cisi_data;
        isi::file::deserialize(compact_index_path, cisi_data);

        std::vector<isi::classic_index> indices;
        for(auto& p: std::experimental::filesystem::directory_iterator(bloom_2_dir)) {
            if (std::experimental::filesystem::is_directory(p)) {
                for(const std::experimental::filesystem::path& isi_p: std::experimental::filesystem::directory_iterator(p)) {
                    if(isi_p.extension() == isi::file::classic_index_header::file_extension) {
                        isi::classic_index isi;
                        isi::file::deserialize(isi_p, isi);
                        indices.push_back(isi);
                    }
                }
            }
        }

        std::sort(indices.begin(), indices.end(), [](auto& i1, auto& i2) {
            return i1.data().size() < i2.data().size();
        });

        ASSERT_EQ(indices.size(), cisi_data.size());
        for (size_t i = 0; i < indices.size(); i++) {
            ASSERT_EQ(indices[i].data().size(), cisi_data[i].size());
            for (size_t j = 0; j < indices[i].data().size(); j++ ) {
                ASSERT_EQ(indices[i].data()[j], cisi_data[i][j]);
            }
        }
    }
}

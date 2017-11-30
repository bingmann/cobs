#include <gtest/gtest.h>
#include "test_util.hpp"
#include <isi/util/parameters.hpp>

namespace {
    std::experimental::filesystem::path in_dir("test/out/classic_index_construction/input");
    std::experimental::filesystem::path samples_dir(in_dir.string() + "/samples");
    std::experimental::filesystem::path isi_2_dir(in_dir.string() + "/isi_2");
    std::experimental::filesystem::path classic_index_path(in_dir.string() + "/index.cla_idx.isi");
    std::experimental::filesystem::path tmp_dir("test/out/classic_index_construction/tmp");

    std::string query = isi::random_sequence(10000, 1);

    class classic_index_construction : public ::testing::Test {
    protected:
        virtual void SetUp() {
            std::error_code ec;
            std::experimental::filesystem::remove_all(in_dir, ec);
            std::experimental::filesystem::remove_all(tmp_dir, ec);
            std::experimental::filesystem::create_directories(in_dir);
            std::experimental::filesystem::create_directories(tmp_dir);
        }
    };

    TEST_F(classic_index_construction, deserialization) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);

        isi::classic_index::create(tmp_dir, in_dir, 8, 3, 0.1);
        std::vector<uint8_t> data;
        isi::file::classic_index_header h;
        isi::file::deserialize(classic_index_path, data, h);
        ASSERT_EQ(h.file_names().size(), 33U);
        ASSERT_EQ(h.num_hashes(), 3U);
    }

    TEST_F(classic_index_construction, file_names) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);

        std::vector<std::experimental::filesystem::path> paths;
        std::experimental::filesystem::recursive_directory_iterator it(tmp_dir), end;
        std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
            return isi::file::file_is<isi::file::sample_header>(p);
        });
        std::sort(paths.begin(), paths.end());

        isi::classic_index::create(tmp_dir, in_dir, 8, 3, 0.1);
        std::vector<uint8_t> data;
        auto h = isi::file::deserialize_header<isi::file::classic_index_header>(classic_index_path);
        isi::file::deserialize(classic_index_path, data, h);
        for (size_t i = 0; i < h.file_names().size(); i++) {
            ASSERT_EQ(h.file_names()[i], isi::file::file_name(paths[i]));
        }
    }

    TEST_F(classic_index_construction, num_ones) {
        auto samples = generate_samples_all(query);
        generate_test_case(samples, tmp_dir);
        isi::classic_index::create(tmp_dir, in_dir, 8, 3, 0.1);
        std::vector<uint8_t> data;
        isi::file::classic_index_header h;
        isi::file::deserialize(classic_index_path, data, h);

        std::map<std::string, size_t> num_ones;
        for (size_t j = 0; j < h.signature_size(); j++) {
            for (size_t k = 0; k < h.block_size(); k++) {
                uint8_t d = data[j * h.block_size() + k];
                for (size_t o = 0; o < 8; o++) {
                    size_t file_names_index = k * 8 + o;
                    if (file_names_index < h.file_names().size()) {
                        std::string file_name = h.file_names()[file_names_index];
                        num_ones[file_name] += (d & (1 << o)) >> o;
                    }
                }
            }
        }

        double set_bit_ratio = isi::calc_average_set_bit_ratio(h.signature_size(), 3, 0.1);
        double num_ones_average = set_bit_ratio * h.signature_size();
        for (auto& no: num_ones) {
            ASSERT_LE(no.second, num_ones_average * 1.01);
        }
    }
}

#include <gtest/gtest.h>
#include <iostream>
#include <file/header.hpp>
#include <experimental/filesystem>
#include "file/util.hpp"
#include <isi/classic_index.hpp>


namespace {
    std::experimental::filesystem::path out_dir("test/out/file");
    std::experimental::filesystem::path out_path_s(out_dir.string() + "/classic_index.g_sam");
    std::experimental::filesystem::path out_path_isi(out_dir.string() + "/classic_index.g_isi");
    std::experimental::filesystem::path out_path_cisi(out_dir.string() + "/compact_index.g_cisi");

    class file : public ::testing::Test {
    protected:
        virtual void SetUp() {
            std::error_code ec;
            std::experimental::filesystem::remove_all(out_dir, ec);
            std::experimental::filesystem::create_directories(out_dir);
        }
    };

    TEST_F(file, sample) {
        isi::sample<31> s_out;
        isi::file::serialize(out_path_s, s_out);

        isi::sample<31> s_in;
        isi::file::deserialize(out_path_s, s_in);
    }

    TEST_F(file, classic_index) {
        isi::classic_index bf_out(123, 12, 1234);
        isi::file::serialize(out_path_isi, bf_out, std::vector<std::string>(12 * 8));

        isi::classic_index bf_in;
        isi::file::deserialize(out_path_isi, bf_in);
        ASSERT_EQ(bf_out.signature_size(), bf_in.signature_size());
        ASSERT_EQ(bf_out.block_size(), bf_in.block_size());
        ASSERT_EQ(bf_out.num_hashes(), bf_in.num_hashes());
    }

    TEST_F(file, compact_index_header_values) {
        std::vector<isi::file::compact_index_header::parameter> parameters = {
                {100, 1},
                {200, 1},
                {3000, 1},
        };
        std::vector<std::string> file_names = {"file_1", "file_2", "file_3"};
        isi::file::compact_index_header h(parameters, file_names, 4096);

        std::ofstream ofs;
        isi::file::serialize_header(ofs, out_path_cisi, h);
        ofs.close();

        std::ifstream ifs;
        auto h_2 = isi::file::deserialize_header<isi::file::compact_index_header>(ifs, out_path_cisi);

        for (size_t i = 0; i < parameters.size(); i++) {
            ASSERT_EQ(parameters[i].num_hashes, h_2.parameters()[i].num_hashes);
            ASSERT_EQ(parameters[i].signature_size, h_2.parameters()[i].signature_size);
        }
        for (size_t i = 0; i < file_names.size(); i++) {
            ASSERT_EQ(file_names[i], h_2.file_names()[i]);
        }
    }

    TEST_F(file, compact_index_header_padding) {
        std::vector<isi::file::compact_index_header::parameter> parameters = {};
        std::vector<std::string> file_names = {};
        uint64_t page_size = 4096;
        isi::file::compact_index_header h(parameters, file_names, page_size);

        std::ofstream ofs;
        isi::file::serialize_header(ofs, out_path_cisi, h);
        ofs.close();

        std::ifstream ifs;
        isi::file::deserialize_header<isi::file::compact_index_header>(ifs, out_path_cisi);
        isi::stream_metadata smd = isi::get_stream_metadata(ifs);
        ASSERT_EQ(smd.curr_pos % page_size, 0U);
    }
}


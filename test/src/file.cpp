#include <gtest/gtest.h>
#include <iostream>

#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/file/header.hpp>


namespace {
    namespace fs = cobs::fs;

    fs::path out_dir("test/out/file");
    fs::path out_path_s(out_dir.string() + "/classic_index.sam.isi");
    fs::path out_path_isi(out_dir.string() + "/classic_index.cla_idx.isi");
    fs::path out_path_cisi(out_dir.string() + "/compact_index.com_idx.isi");

    class file : public ::testing::Test {
    protected:
        virtual void SetUp() {
            cobs::error_code ec;
            fs::remove_all(out_dir, ec);
            fs::create_directories(out_dir);
        }
    };

    TEST_F(file, sample) {
        cobs::sample<31> s_out;
        cobs::file::serialize(out_path_s, s_out, "sample");

        cobs::sample<31> s_in;
        cobs::file::deserialize(out_path_s, s_in);
    }

    TEST_F(file, classic_index) {
        std::vector<std::string> file_names = {"n1", "n2", "n3", "n4"};
        cobs::file::classic_index_header h_out(123, 12, file_names);
        std::vector<uint8_t> v_out(h_out.block_size() * h_out.signature_size(), 7);
        cobs::file::serialize(out_path_isi, v_out, h_out);

        cobs::file::classic_index_header h_in;
        std::vector<uint8_t> v_in;
        cobs::file::deserialize(out_path_isi, v_in, h_in);
        ASSERT_EQ(h_out.signature_size(), h_in.signature_size());
        ASSERT_EQ(h_out.block_size(), h_in.block_size());
        ASSERT_EQ(h_out.num_hashes(), h_in.num_hashes());
        ASSERT_EQ(v_out.size(), v_in.size());
        for (size_t i = 0; i < v_out.size(); i++) {
            ASSERT_EQ(v_out[i], v_in[i]);
        }
        ASSERT_EQ(file_names.size(), h_in.file_names().size());
        for(size_t i = 0; i < file_names.size(); i++) {
            ASSERT_EQ(file_names[i], h_in.file_names()[i]);
        }
    }

    TEST_F(file, classic_index_header) {
        std::vector<std::string> file_names = {"n1", "n2", "n3", "n4"};
        cobs::file::classic_index_header h_out(321, 21, file_names);
        cobs::file::serialize_header(out_path_isi, h_out);

        auto h_in = cobs::file::deserialize_header<cobs::file::classic_index_header>(out_path_isi);
        ASSERT_EQ(h_out.signature_size(), h_in.signature_size());
        ASSERT_EQ(h_out.block_size(), h_in.block_size());
        ASSERT_EQ(h_out.num_hashes(), h_in.num_hashes());
        ASSERT_EQ(file_names.size(), h_in.file_names().size());
        for(size_t i = 0; i < file_names.size(); i++) {
            ASSERT_EQ(file_names[i], h_in.file_names()[i]);
        }
    }

    TEST_F(file, compact_index_header_values) {
        std::vector<cobs::file::compact_index_header::parameter> parameters = {
                {100, 1},
                {200, 1},
                {3000, 1},
        };
        std::vector<std::string> file_names = {"file_1", "file_2", "file_3"};
        cobs::file::compact_index_header h(parameters, file_names, 4096);
        cobs::file::serialize_header(out_path_cisi, h);

        auto h_2 = cobs::file::deserialize_header<cobs::file::compact_index_header>(out_path_cisi);

        for (size_t i = 0; i < parameters.size(); i++) {
            ASSERT_EQ(parameters[i].num_hashes, h_2.parameters()[i].num_hashes);
            ASSERT_EQ(parameters[i].signature_size, h_2.parameters()[i].signature_size);
        }
        for (size_t i = 0; i < file_names.size(); i++) {
            ASSERT_EQ(file_names[i], h_2.file_names()[i]);
        }
    }

    TEST_F(file, compact_index_header_padding) {
        std::vector<cobs::file::compact_index_header::parameter> parameters = {};
        std::vector<std::string> file_names = {};
        uint64_t page_size = 4096;
        cobs::file::compact_index_header h(parameters, file_names, page_size);
        cobs::file::serialize_header(out_path_cisi, h);

        std::ifstream ifs;
        cobs::file::deserialize_header<cobs::file::compact_index_header>(ifs, out_path_cisi);
        cobs::stream_metadata smd = cobs::get_stream_metadata(ifs);
        ASSERT_EQ(smd.curr_pos % page_size, 0U);
    }
}


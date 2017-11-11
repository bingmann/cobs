#include <gtest/gtest.h>
#include <iostream>
#include <file/header.hpp>
#include <experimental/filesystem>
#include <file/util.hpp>
#include <isi/classic_index.hpp>


namespace {
    std::string out_dir = "test/out/file/";
    std::string out_path_s = out_dir + "classic_index.g_sam";
    std::string out_path_bf = out_dir + "classic_index.g_isi";

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
        isi::file::serialize(out_path_bf, bf_out, std::vector<std::string>(12 * 8));

        isi::classic_index bf_in;
        isi::file::deserialize(out_path_bf, bf_in);
        ASSERT_EQ(bf_out.signature_size(), bf_in.signature_size());
        ASSERT_EQ(bf_out.block_size(), bf_in.block_size());
        ASSERT_EQ(bf_out.num_hashes(), bf_in.num_hashes());
    }
}

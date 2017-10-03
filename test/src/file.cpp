#include <gtest/gtest.h>
#include <iostream>
#include <file/header.hpp>
#include <boost/filesystem.hpp>
#include <file/util.hpp>
#include <bloom_filter.hpp>


namespace {
    using namespace genome::file;

    std::string out_dir = "test/out/file/";
    std::string out_path_s = out_dir + "bloom_filter.g_sam";
    std::string out_path_bf = out_dir + "bloom_filter.g_blo";

    class file : public ::testing::Test {
    protected:
        virtual void SetUp() {
            boost::filesystem::create_directories(out_dir);
        }

        virtual void TearDown() {
            boost::filesystem::remove_all(out_dir);
        }
    };

    TEST_F(file, sample) {
        genome::sample<31> s_out;
        serialize(out_path_s, s_out);

        genome::sample<31> s_in;
        deserialize(out_path_s, s_in);
    }

    TEST_F(file, bloom_filter) {
        genome::bloom_filter bf_out(123, 12, 1234);
        serialize(out_path_bf, bf_out);

        genome::bloom_filter bf_in;
        deserialize(out_path_bf, bf_in);
        ASSERT_EQ(bf_out.bloom_filter_size(), bf_in.bloom_filter_size());
        ASSERT_EQ(bf_out.block_size(), bf_in.block_size());
        ASSERT_EQ(bf_out.num_hashes(), bf_in.num_hashes());
    }
}

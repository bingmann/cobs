#include <gtest/gtest.h>
#include <iostream>
#include <file/header.hpp>
#include <file/bloom_filter_header.hpp>
#include <boost/filesystem.hpp>


namespace {
    using namespace genome::file;

    std::string out_dir = "test/out/file/";
    std::string out_path = out_dir + "bloom_filter.h";

    class file : public ::testing::Test {
    protected:
        virtual void SetUp() {
            boost::filesystem::create_directories(out_dir);
        }

        virtual void TearDown() {
            boost::filesystem::remove_all(out_dir);
        }
    };

    TEST_F(file, bloom_filter) {
        bloom_filter_header out(1234567, 1234321, 7654321);
        std::ofstream ofs(out_path, std::ios::out | std::ios::binary);
        header<bloom_filter_header>::serialize(ofs, out);
        ofs.flush();
        ofs.close();

        std::ifstream ifs(out_path, std::ios::in | std::ios::binary);
        bloom_filter_header in;
        header<bloom_filter_header>::deserialize(ifs, in);
        ASSERT_EQ(out.bloom_filter_size(), in.bloom_filter_size());
        ASSERT_EQ(out.block_size(), in.block_size());
        ASSERT_EQ(out.num_hashes(), in.num_hashes());
    }
}

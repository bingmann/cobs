#include <string>
#include <fstream>
#include <vector>
#include <gtest/gtest.h>

void assert_equals_files(const std::string& f1, const std::string& f2) {
    std::ifstream ifs1(f1, std::ios::in | std::ios::binary);
    std::ifstream ifs2(f2, std::ios::in | std::ios::binary);
    std::istream_iterator<char> start1(ifs1);
    std::istream_iterator<char> start2(ifs2);
    std::istream_iterator<char> end;
    std::vector<char> v1(start1, end);
    std::vector<char> v2(start2, end);

    ASSERT_EQ(v1.size(), v2.size());
    for (size_t i = 0; i < v1.size(); i++) {
        ASSERT_EQ(v1[i], v2[i]);
    }
}

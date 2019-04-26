/*******************************************************************************
 * src/exp_fp_dist.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/query/classic_search.hpp>
#include <cobs/query/compact_index/mmap_search_file.hpp>
#include <cobs/util/misc.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::array<uint32_t, 6> indices = { 4096, 8192, 12288, 16384, 28672, 57344 };

    std::string document_name = "ERR498185";
    std::vector<std::string> queries;
    for (size_t i = 0; i < 1; i++) {
        queries.push_back(cobs::random_sequence(1030, (size_t)time(nullptr)));
    }
    std::vector<std::pair<uint16_t, std::string> > result;

    for (uint32_t in : indices) {
        std::string index = "/well/iqbal/people/florian/indices/ena_" + std::to_string(in) + ".cobs_compact";

        std::map<size_t, size_t> counts;
        cobs::CompactIndexMMapSearchFile s_mmap(index);
        cobs::ClassicSearch s(s_mmap);
        for (size_t i = 0; i < queries.size(); i++) {
            s.search(queries[i], result);
            for (const auto& r : result) {
                counts[r.first]++;
            }
        }
        for (auto a : counts) {
            std::cout << a.first << "," << a.second << "," << in << "\n";
        }
    }
}

/******************************************************************************/

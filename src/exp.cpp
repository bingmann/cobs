/*******************************************************************************
 * src/exp.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cmath>
#include <ctime>
#include <map>
#include <unistd.h>

#include <cobs/query/classic_index/mmap.hpp>
#include <cobs/query/compact_index/mmap.hpp>

#ifndef NO_AIO
#include <cobs/query/compact_index/aio.hpp>
#endif

#include <tlx/cmdline_parser.hpp>

#define FALSE_POSITIVE_DIST

void run(const cobs::fs::path& p,
         const std::vector<std::string>& queries,
         const std::vector<std::string>& warump_queries) {
    cobs::query::classic_index::mmap s(p);
#ifndef NO_AIO
    // cobs::query::compact_index::aio s(p);
#endif
    sync();
    std::ofstream ofs("/proc/sys/vm/drop_caches");
    ofs << "3" << std::endl;
    ofs.close();

    std::vector<std::pair<uint16_t, std::string> > result;
    for (size_t i = 0; i < warump_queries.size(); i++) {
        s.search(warump_queries[i], 31, result);
    }
    s.get_timer().reset();

#ifdef FALSE_POSITIVE_DIST
    std::map<uint16_t, double> counts;
#endif

    for (size_t i = 0; i < queries.size(); i++) {
        s.search(queries[i], 31, result);
#ifdef FALSE_POSITIVE_DIST
        for (const auto& r : result) {
            counts[r.first]++;
        }
#endif
    }

    std::string simd = "on";
    std::string openmp = "on";
    std::string aio = "on";

#ifdef NO_SIMD
    simd = "off";
#endif

#ifdef NO_OPENMP
    openmp = "off";
#endif

#ifdef NO_AIO
    aio = "off";
#endif

    cobs::timer t = s.get_timer();
    std::cout << "RESULT"
              << " index=" << p.string()
              << " kmers=" << (queries[0].size() - 30)
              << " queries=" << queries.size()
              << " warmup=" << warump_queries.size()
              << " simd=" << simd
              << " openmp=" << openmp
              << " aio=" << aio
              << " t_hashes=" << t.get("hashes")
              << " t_io=" << t.get("io")
              << " t_and=" << t.get("and rows")
              << " t_add=" << t.get("add rows")
              << " t_sort=" << t.get("sort results");
#ifdef FALSE_POSITIVE_DIST
    for (const auto& c : counts) {
        std::cout << " c_" << c.first << "="
                  << (c.second / queries.size() / result.size());
    }
#endif
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;

    std::string in_file;
    cp.add_param_string(
        "in_file", in_file, "path to the input file");

    unsigned num_kmers = 100;
    cp.add_unsigned(
        'k', "num_kmers", num_kmers,
        "number of kmers of each query, default: 100");

    unsigned num_random = 100;
    cp.add_unsigned(
        'r', "random", num_random,
        "number of random queries to run, default: 100");

    unsigned num_warmup = 100;
    cp.add_unsigned(
        'w', "warmup", num_warmup,
        "number of random warmup queries to run, default: 100");

    if (!cp.process(argc, argv))
        return -1;

    std::vector<std::string> warmup_queries;
    std::vector<std::string> queries;
    for (size_t i = 0; i < num_warmup; i++) {
        warmup_queries.push_back(
            cobs::random_sequence(num_kmers + 30, (size_t)time(nullptr)));
    }
    for (size_t i = 0; i < num_random; i++) {
        queries.push_back(
            cobs::random_sequence(num_kmers + 30, (size_t)time(nullptr)));
    }
    run(in_file, queries, warmup_queries);

    return 0;
}

/******************************************************************************/

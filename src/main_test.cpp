#include <array>
#include <iostream>
#include <vector>
#include <helpers.hpp>
#include <stdexcept>
#include <kmer.hpp>
#include <sample.hpp>
#include <cortex.hpp>


int main(int argc, char** argv) {
    sample<31> s;
    cortex::time_stats ts;
    cortex::process_file("/users/flo/projects/thesis/data/performance/ERR108326/uncleaned/ERR108326.ctx", "abc.txt", s, ts);
}

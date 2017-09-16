#pragma once

#include <string>
#include <vector>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>

#include "helpers.hpp"
#include "sample.hpp"

namespace cortex {
    template<unsigned int N>
    void format_sample(const std::string& cmd, sample<N>& sample) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) throw std::runtime_error("popen() failed!");
        try {
            sample.init([&](char* buffer, unsigned int buffer_size) {
                return !feof(pipe) && fgets(buffer, buffer_size, pipe) != nullptr;
            });
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);
    }

    template<unsigned int N>
    void serialize(const sample<N>& sample, const boost::filesystem::path& path) {
        boost::filesystem::create_directories(path.parent_path());
        std::ofstream ofs(path.string(), std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(sample.data().data()), kmer<N>::size * sample.data().size());
    }

    template<unsigned int N>
    void deserialize(sample<N>& sample, const boost::filesystem::path& path) {
        std::ifstream ifs(path.string(), std::ios::in | std::ios::binary);
        ifs.seekg(0, std::ios_base::end);
        int64_t size = ifs.tellg();
        ifs.seekg(0, std::ios_base::beg);
        sample.data().resize(size / kmer<N>::size);
        ifs.read(reinterpret_cast<char*>(sample.data().data()), size);
        ifs.close();
    }

    template<unsigned int N>
    void process_file(const boost::filesystem::path& in_path, const boost::filesystem::path& out_path, sample<N>& sample) {
        std::string cmd = "mccortex31 view -q -k " + in_path.string();
        format_sample(cmd, sample);
        serialize(sample, out_path);
    }

    template<unsigned int N>
    void process_all_in_directory(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir) {
        sample<N> sample;
        for(boost::filesystem::recursive_directory_iterator end, it(in_dir); it != end; it++) {
            if(boost::filesystem::is_regular_file(*it) && it->path().extension() == ".ctx") {
                process_file(it->path(), out_dir / it->path().stem().concat(".b"), sample);
            }
        }
    }
}



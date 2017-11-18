#pragma once

#include <experimental/filesystem>
#include "file/sample_header.hpp"
#include "file/frequency_header.hpp"
#include "file/util.hpp"
#include "timer.hpp"
#include "util.hpp"
#include <iostream>
#include <queue>

namespace isi::frequency {
    template<typename H>
    class pq_element {
    protected:
        std::ifstream* m_ifs;
        uint64_t m_kmer;
        uint32_t m_count;

    public:
        explicit pq_element(std::ifstream* ifs) : m_ifs(ifs) {
            ifs->read(reinterpret_cast<char*>(&m_kmer), 8);
        }

        static std::string file_extension() {
            return H::file_extension;
        }

        static void serialize_header(std::ofstream& ofs, const std::experimental::filesystem::path& p) {
            file::serialize_header<H>(ofs, p);
        }

        static void deserialize_header(std::ifstream& ifs, const std::experimental::filesystem::path& p) {
            file::deserialize_header<H>(ifs, p);
        }

        static bool comp(const pq_element& bs1, const pq_element& bs2) {
            return bs1.m_kmer > bs2.m_kmer;
        }

        std::ifstream* ifs() {
            return m_ifs;
        }

        uint64_t kmer() {
            return m_kmer;
        }

        virtual uint32_t count() {
            return m_count;
        }
    };

    class bin_pq_element : public pq_element<file::sample_header> {
    public:
        explicit bin_pq_element(std::ifstream* ifs) : pq_element(ifs) {
            m_count = 1;
        }
    };

    class fre_pq_element : public pq_element<file::frequency_header> {
    public:
        explicit fre_pq_element(std::ifstream* ifs) : pq_element(ifs) {
            ifs->read(reinterpret_cast<char*>(&m_count), 4);
        }
    };

    template<typename PqElement>
    void add_pq_element(std::priority_queue<PqElement, std::vector<PqElement>, std::function<bool(const PqElement&, const PqElement&)>>& pq, std::ifstream* ifs) {
        if (ifs && ifs->peek() != EOF) {
            pq.push(PqElement(ifs));
        }
    }

    template<typename PqElement>
    void process(std::vector<std::ifstream>& ifstreams, const std::experimental::filesystem::path& out_file) {
        std::priority_queue<PqElement, std::vector<PqElement>, std::function<bool(const PqElement&, const PqElement&)>> pq(PqElement::comp);
        std::ofstream ofs;
        file::serialize_header<file::frequency_header>(ofs, out_file, file::frequency_header());

        for(auto& ifs: ifstreams) {
            add_pq_element(pq, &ifs);
        }

        auto pq_element = pq.top();
        pq.pop();
        uint64_t kmer = pq_element.kmer();
        uint32_t count = pq_element.count();
        add_pq_element(pq, pq_element.ifs());

        while(!pq.empty()) {
            pq_element = pq.top();
            pq.pop();
            if (pq_element.kmer() == kmer) {
                count += pq_element.count();
            } else {
                ofs.write(reinterpret_cast<const char*>(&kmer), 8);
                ofs.write(reinterpret_cast<const char*>(&count), 4);
                kmer = pq_element.kmer();
                count = pq_element.count();
            }
            add_pq_element(pq, pq_element.ifs());
        }
        ofs.write(reinterpret_cast<const char*>(&kmer), 8);
        ofs.write(reinterpret_cast<const char*>(&count), 4);
        ofs.flush();
        ofs.close();
    }

    template<typename PqElement>
    void process_all_in_directory(const std::experimental::filesystem::path& in_dir, const std::experimental::filesystem::path& out_dir, size_t batch_size) {
        timer t;
        t.active("process");
        std::vector<std::ifstream> ifstreams;
        bulk_process_files(in_dir, out_dir, batch_size, PqElement::file_extension(), file::frequency_header::file_extension,
                           [&](const std::vector<std::experimental::filesystem::path>& paths, const std::experimental::filesystem::path& out_file) {
                               for (const auto& p: paths) {
                                   ifstreams.emplace_back(std::ifstream());
                                   PqElement::deserialize_header(ifstreams.back(), p);
                               }
                               process<PqElement>(ifstreams, out_file);
                               ifstreams.clear();
                           });
        t.stop();
        std::cout << t;
    }

    inline void combine(const std::experimental::filesystem::path& in_file, const std::experimental::filesystem::path& out_file) {
        std::ifstream ifs;
        file::deserialize_header<file::frequency_header>(ifs, in_file);
        std::unordered_map<uint32_t, uint32_t> counts;

        std::array<char, 12> data;
        uint32_t* count = reinterpret_cast<uint32_t*>(data.data() + 8);
        while (ifs && ifs.peek() != EOF) {
            ifs.read(data.data(), 12);
            counts[*count]++;
        }
        std::ofstream ofs(out_file.string());
        for (const auto& count: counts) {
            ofs << count.first << "," << count.second << "\n";
        }
    }
};

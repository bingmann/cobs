#include "frequency.hpp"
#include "timer.hpp"
#include "util.hpp"
#include <iostream>
#include <queue>
#include <file/util.hpp>

namespace genome::frequency {


    template<typename PqElement>
    void add_pq_element(std::priority_queue<PqElement, std::vector<PqElement>, std::function<bool(const PqElement&, const PqElement&)>>& pq, std::ifstream* ifs) {
        if (ifs && ifs->peek() != EOF) {
            pq.push(PqElement(ifs));
        }
    }

    template<typename PqElement>
    void process(std::vector<std::ifstream>& ifstreams, const boost::filesystem::path& out_file) {
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
    void process_all_in_directory(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir, size_t batch_size) {
        timer t;
        t.active("process");
        std::vector<std::ifstream> ifstreams;
        bulk_process_files(in_dir, out_dir, batch_size, PqElement::file_extension(), file::frequency_header::file_extension,
        [&](const std::vector<boost::filesystem::path>& paths, const boost::filesystem::path& out_file) {
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

    inline void combine(const boost::filesystem::path& in_file, const boost::filesystem::path& out_file) {
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
}

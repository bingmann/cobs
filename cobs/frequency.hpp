/*******************************************************************************
 * cobs/frequency.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_FREQUENCY_HEADER
#define COBS_FREQUENCY_HEADER

#include <cobs/file/document_header.hpp>
#include <cobs/file/frequency_header.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/processing.hpp>
#include <cobs/util/timer.hpp>

#include <iostream>
#include <queue>

namespace cobs::frequency {

template <typename H>
class pq_element
{
protected:
    std::ifstream* m_ifs;
    uint64_t m_kmer;
    uint32_t m_count;

public:
    explicit pq_element(std::ifstream* ifs);
    static void serialize_header(std::ofstream& ofs, const fs::path& p);
    static void deserialize_header(std::ifstream& ifs, const fs::path& p);
    std::ifstream * ifs();
    uint64_t kmer();
    uint32_t count();

    bool operator < (const pq_element& b) const {
        return m_kmer > b.m_kmer;
    }
};

class bin_pq_element : public pq_element<DocumentHeader>
{
public:
    explicit bin_pq_element(std::ifstream* ifs);
};

class fre_pq_element : public pq_element<FrequencyHeader>
{
public:
    explicit fre_pq_element(std::ifstream* ifs);
};

template <typename H, typename PQE = typename std::conditional<std::is_same<H, DocumentHeader>::value, bin_pq_element, fre_pq_element>::type>
void process_all_in_directory(const fs::path& in_dir, const fs::path& out_dir, size_t batch_size);
inline void combine(const fs::path& in_file, const fs::path& out_file);

template <typename H>
pq_element<H>::pq_element(std::ifstream* ifs) : m_ifs(ifs) {
    ifs->read(reinterpret_cast<char*>(&m_kmer), 8);
}

template <typename H>
void pq_element<H>::serialize_header(std::ofstream& ofs, const fs::path& p) {
    file::serialize_header<H>(ofs, p);
}

template <typename H>
void pq_element<H>::deserialize_header(std::ifstream& ifs, const fs::path& p) {
    file::deserialize_header<H>(ifs, p);
}

template <typename H>
std::ifstream* pq_element<H>::ifs() {
    return m_ifs;
}

template <typename H>
uint64_t pq_element<H>::kmer() {
    return m_kmer;
}

template <typename H>
uint32_t pq_element<H>::count() {
    return m_count;
}

bin_pq_element::bin_pq_element(std::ifstream* ifs) : pq_element(ifs) {
    m_count = 1;
}

fre_pq_element::fre_pq_element(std::ifstream* ifs) : pq_element(ifs) {
    ifs->read(reinterpret_cast<char*>(&m_count), 4);
}

template <typename PqElement>
void add_pq_element(std::priority_queue<PqElement>& pq,
                    std::ifstream* ifs) {
    if (ifs && ifs->peek() != EOF) {
        pq.push(PqElement(ifs));
    }
}

template <typename PqElement>
void process(std::vector<std::ifstream>& ifstreams, const fs::path& out_file) {
    std::priority_queue<PqElement> pq;
    std::ofstream ofs;
    file::serialize_header<FrequencyHeader>(ofs, out_file, FrequencyHeader());

    for (auto& ifs : ifstreams) {
        add_pq_element(pq, &ifs);
    }

    PqElement pqe = pq.top();
    pq.pop();
    uint64_t kmer = pqe.kmer();
    uint32_t count = pqe.count();
    add_pq_element(pq, pqe.ifs());

    while (!pq.empty()) {
        pqe = pq.top();
        pq.pop();
        if (pqe.kmer() == kmer) {
            count += pqe.count();
        }
        else {
            ofs.write(reinterpret_cast<const char*>(&kmer), 8);
            ofs.write(reinterpret_cast<const char*>(&count), 4);
            kmer = pqe.kmer();
            count = pqe.count();
        }
        add_pq_element(pq, pqe.ifs());
    }
    ofs.write(reinterpret_cast<const char*>(&kmer), 8);
    ofs.write(reinterpret_cast<const char*>(&count), 4);
}

template <typename H, typename PQE>
void process_all_in_directory(const fs::path& in_dir, const fs::path& out_dir, size_t batch_size) {
    Timer t;
    t.active("process");
    std::vector<std::ifstream> ifstreams;
    process_file_batches(
        in_dir, out_dir, batch_size, cobs::FrequencyHeader::file_extension,
        [](const fs::path& path) {
            return file::file_is<H>(path);
        },
        [&](const std::vector<fs::path>& paths, const fs::path& out_file) {
            for (const auto& p : paths) {
                ifstreams.emplace_back(std::ifstream());
                PQE::deserialize_header(ifstreams.back(), p);
                ifstreams.back().exceptions(std::ios::failbit | std::ios::badbit);
            }
            process<PQE>(ifstreams, out_file);
            ifstreams.clear();
        });
    t.stop();
    std::cout << t;
}

inline void combine(const fs::path& in_file, const fs::path& out_file) {
    std::ifstream ifs;
    file::deserialize_header<FrequencyHeader>(ifs, in_file);
    std::unordered_map<uint32_t, uint32_t> counts;

    std::array<char, 12> data;
    uint32_t* count = reinterpret_cast<uint32_t*>(data.data() + 8);
    while (ifs && ifs.peek() != EOF) {
        ifs.read(data.data(), 12);
        counts[*count]++;
    }
    std::ofstream ofs(out_file.string());
    for (const auto& count : counts) {
        ofs << count.first << "," << count.second << "\n";
    }
}

} // namespace cobs::frequency

#endif // !COBS_FREQUENCY_HEADER

/******************************************************************************/

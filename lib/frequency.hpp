#pragma once

#include <boost/filesystem.hpp>

namespace frequency {
//    template<typename PqElement>
    void process_all_in_directory(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir);

    class bin_pq_element {
    private:
        std::ifstream* m_ifs;
        uint64_t m_kmer;

    public:
        explicit bin_pq_element(std::ifstream* ifs) : m_ifs(ifs){
            ifs->read(reinterpret_cast<char*>(&m_kmer), 8);
        }
        std::ifstream* ifs() {
            return m_ifs;
        }
        uint64_t  kmer() {
            return m_kmer;
        }
        virtual uint32_t count() {
            return 1;
        }
        static bool comp(const bin_pq_element& bs1, const bin_pq_element& bs2) {
            return bs1.m_kmer > bs2.m_kmer;
        }
        static std::string file_extension() {
            return ".b";
        }
    };

    class freq_pq_element : public bin_pq_element {
        uint32_t m_count;
    public:
        explicit freq_pq_element(std::ifstream *ifs) : bin_pq_element(ifs) {
            ifs->read(reinterpret_cast<char*>(&m_count), 4);
        }
        uint32_t count() override {
            return m_count;
        }
        static std::string file_extension() {
            return ".f";
        }
    };

};

#include "frequency.tpp"

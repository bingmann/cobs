#pragma once

#include <boost/filesystem.hpp>
#include "file/sample_header.hpp"
#include "file/frequency_header.hpp"
#include "file/util.hpp"

namespace genome::frequency {
    void process_all_in_directory(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir);

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

        static void serialize_header(std::ofstream& ofs, const boost::filesystem::path& p) {
            file::serialize_header<H>(ofs, p);
        }

        static void deserialize_header(std::ifstream& ifs, const boost::filesystem::path& p) {
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
};

#include "frequency.tpp"

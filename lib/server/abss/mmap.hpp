#pragma once


#include <server/abss/base.hpp>

namespace genome::server::abss {
    class mmap : public base {
    private:
        byte* m_data;
        virtual void read_from_disk(const std::vector<size_t>& hashes, char* rows);
        void aggregate_rows(const std::vector<size_t>& hashes, std::vector<char>& rows);
        void compute_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts, const std::vector<char>& rows);
    protected:
        void calculate_counts(const std::vector<size_t>& hashes, std::vector<uint16_t>& counts) override;
    public:
        explicit mmap(const boost::filesystem::path& path);
    };
}

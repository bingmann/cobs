#pragma once

#include <boost/filesystem.hpp>
#include <helpers.hpp>
#include <kmer.hpp>
#include <timer.hpp>

namespace genome {
    class bloom_filter {
    private:
        uint64_t m_bloom_filter_size;
        uint64_t m_block_size;
        uint64_t m_num_hashes;
        std::vector<byte> m_data;

        void set_bit(size_t pos, size_t bit_in_block);
        bool is_set(size_t pos, size_t bit_in_block);
        void process(const std::vector<boost::filesystem::path> &paths, const boost::filesystem::path &out_file, timer &t);
        static void combine(std::vector<std::pair<std::ifstream, size_t>>& ifstreams, const boost::filesystem::path &out_file,
                            size_t bloom_filter_size, size_t block_size, size_t num_hash, timer& t);
    public:
        bloom_filter() = default;
        bloom_filter(uint64_t bloom_filter_size, uint64_t block_size, uint64_t num_hashes);
        static void create_from_samples(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir,
                                                     size_t bloom_filter_size, size_t block_size, size_t num_hashes);
        static void combine_bloom_filters(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir,
                                                     size_t batch_size, size_t bloom_filter_size, size_t num_hashes);
        bool contains(const kmer<31>& kmer, size_t bit_in_block);
        uint64_t bloom_filter_size() const;
        void bloom_filter_size(uint64_t m_bloom_filter_size);
        uint64_t block_size() const;
        void block_size(uint64_t m_block_size);
        uint64_t num_hashes() const;
        void num_hashes(uint64_t m_num_hashes);
        const std::vector<byte>& data() const;
        std::vector<byte>& data();
    };
}

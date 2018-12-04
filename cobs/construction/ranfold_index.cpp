/*******************************************************************************
 * cobs/construction/ranfold_index.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <random>
#include <set>

#include <cobs/construction/classic_index.hpp>
#include <cobs/construction/ranfold_index.hpp>
#include <cobs/cortex.hpp>
#include <cobs/document.hpp>
#include <cobs/file/ranfold_index_header.hpp>
#include <cobs/kmer.hpp>
#include <cobs/util/addressable_priority_queue.hpp>
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/parameters.hpp>
#include <cobs/util/processing.hpp>
#include <cobs/util/timer.hpp>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>
#include <tlx/math/popcount.hpp>

namespace cobs::ranfold_index {

void set_bit(std::vector<uint8_t>& data,
             const file::ranfold_index_header& rih,
             uint64_t term_index, uint64_t document_index) {
    size_t x = rih.m_doc_space_bytes * term_index + document_index / 8;
    die_unless(x < data.size());
    data[x] |= 1 << (document_index % 8);
}

template <typename RandomGenerator>
document<31>& document_generator(
    document<31>& doc, size_t document_size, RandomGenerator& rng) {

    // create random document
    doc.data().clear();
    for (size_t j = 0; j < document_size; ++j) {
        kmer<31> m;
        // should canonicalize the kmer, but since we can assume uniform
        // hashing, this is not needed.
        m.fill_random(rng);
        doc.data().push_back(m);
    }

    return doc;
}

void mark_document(const document<31>& doc,
                   const file::ranfold_index_header& rih,
                   std::vector<uint8_t>& data,
                   size_t document_index) {
    for (size_t j = 0; j < doc.data().size(); ++j) {
        // process term hashes
        classic_index::process_hashes(
            doc.data().data() + j, 8,
            rih.m_term_space, rih.m_term_hashes,
            [&](uint64_t hash) {
                set_bit(data, rih, hash, document_index);
            });
    }
}

struct Sketch {
    std::vector<uint32_t> hashes;

    void clear() {
        std::vector<uint32_t>().swap(hashes);
    }

    double dist_jaccard(const Sketch& other) const {
        // estimate Jaccard index
        auto ai = hashes.begin(), bi = other.hashes.begin();

        // merge lists to calculate matching elements
        size_t num = 0, denom = 0;
        while (ai != hashes.end() && bi != other.hashes.end())
        {
            if (*ai == *bi) {
                num += 1, denom += 1;
                ++ai, ++bi;
            }
            else if (*ai < *bi) {
                denom += 1;
                ++ai;
            }
            else {
                denom += 1;
                ++bi;
            }
        }

        // remaining elements do not match.
        denom += (hashes.end() - ai) + (other.hashes.end() - bi);

        // sLOG1 << "num" << num << "denom" << denom;
        return num / static_cast<double>(denom);
    }

    //! merge two sketches and return all hashes in both
    Sketch merge_complete(const Sketch& other) const {
        Sketch out;
        out.hashes.reserve(hashes.size() + other.hashes.size());

        auto ai = hashes.begin(), bi = other.hashes.begin();

        // merge lists to calculate elements
        while (ai != hashes.end() && bi != other.hashes.end()) {
            if (*ai == *bi) {
                out.hashes.emplace_back(*ai);
                ++ai, ++bi;
            }
            else if (*ai < *bi) {
                out.hashes.emplace_back(*ai);
                ++ai;
            }
            else {
                out.hashes.emplace_back(*bi);
                ++bi;
            }
        }

        while (ai != hashes.end())
            out.hashes.emplace_back(*ai++);
        while (bi != other.hashes.end())
            out.hashes.emplace_back(*bi++);

        return out;
    }

    //! merge two sketches and return the smallest hashes in both
    Sketch merge(const Sketch& other, size_t limit) const {
        Sketch out;
        out.hashes.reserve(
            std::min(limit, hashes.size() + other.hashes.size()));

        auto ai = hashes.begin(), bi = other.hashes.begin();

        // merge lists to calculate elements
        while (ai != hashes.end() && bi != other.hashes.end() &&
               out.hashes.size() < limit)
        {
            if (*ai == *bi) {
                out.hashes.emplace_back(*ai);
                ++ai, ++bi;
            }
            else if (*ai < *bi) {
                out.hashes.emplace_back(*ai);
                ++ai;
            }
            else {
                out.hashes.emplace_back(*bi);
                ++bi;
            }
        }

        while (ai != hashes.end() && out.hashes.size() < limit)
            out.hashes.emplace_back(*ai++);
        while (bi != other.hashes.end() && out.hashes.size() < limit)
            out.hashes.emplace_back(*bi++);

        return out;
    }

    //! estimate density by returning the largest hash
    double dist_density(const Sketch& other, size_t limit) const {
        Sketch tmp = merge(other, limit);
        return tmp.hashes.back();
    }
};

//! Class to combine many sketches at once.
struct CollectSketch {
    // set to collect hashes in order
    std::set<uint32_t> hashes_;

    void add(const Sketch& other) {
        for (const uint32_t& h : other.hashes) {
            hashes_.insert(h);
        }
    }

    size_t size() const { return hashes_.size(); }

    void get_complete(Sketch* out) const {
        out->hashes.clear();
        out->hashes.reserve(hashes_.size());
        for (const uint32_t& h : hashes_) {
            out->hashes.push_back(h);
        }
    }

    void get(Sketch* out, size_t num_hashes) const {
        out->hashes.clear();
        out->hashes.reserve(num_hashes);
        for (const uint32_t& h : hashes_) {
            out->hashes.push_back(h);
            if (out->hashes.size() >= num_hashes)
                break;
        }
    }

    Sketch get_complete() const {
        Sketch mh;
        get_complete(&mh);
        return mh;
    }

    Sketch get(size_t num_hashes) const {
        Sketch mh;
        get(&mh, num_hashes);
        return mh;
    }
};

void sketch_document(const document<31>& doc,
                     const file::ranfold_index_header& rih,
                     size_t num_hashes,
                     Sketch* min_hash) {

    std::priority_queue<uint64_t> pq;
    std::set<uint64_t> pq_uniq;

    for (size_t j = 0; j < doc.data().size(); ++j) {
        // process term hashes
        classic_index::process_hashes(
            doc.data().data() + j, 8,
            rih.m_term_space, rih.m_term_hashes,
            [&](uint64_t hash) {
                if (pq.size() < num_hashes) {
                    if (pq_uniq.count(hash) == 0) {
                        pq.push(hash);
                        pq_uniq.insert(hash);
                    }
                }
                else if (pq.top() > hash && pq_uniq.count(hash) == 0) {
                    pq_uniq.erase(pq.top());
                    pq.pop();

                    pq.push(hash);
                    pq_uniq.insert(hash);
                }
            });
    }

    min_hash->hashes.resize(pq.size());
    size_t i = pq.size();
    while (!pq.empty()) {
        min_hash->hashes[--i] = pq.top();
        pq.pop();
    }
}

void sketch_path(const fs::path& path,
                 document<31>& doc,
                 const file::ranfold_index_header& rih,
                 size_t num_hashes,
                 Sketch* min_hash) {

    fs::path mh_cache = path.string() + ".sketch_" + std::to_string(num_hashes);
    if (fs::is_regular_file(mh_cache) &&
        fs::last_write_time(path) < fs::last_write_time(mh_cache))
    {
        LOG1 << "Sketching " << path.string() << " [cached]";

        std::ifstream in(mh_cache.string());
        die_unless(in.good());
        in.seekg(0, std::ios::end);
        size_t size = in.tellg();
        if (size / sizeof(min_hash->hashes[0]) == num_hashes) {
            min_hash->hashes.resize(size / sizeof(min_hash->hashes[0]));
            in.seekg(0);
            in.read(reinterpret_cast<char*>(min_hash->hashes.data()), size);
            die_unequal(size, in.gcount());
            return;
        }
    }

    LOG1 << "Sketching " << path.string();
    cortex::cortex_file ctx(path.string());
    doc.data().clear();
    ctx.process_kmers<31>(
        [&](kmer<31>& m) {
            m.canonicalize();
            doc.data().push_back(m);
        });

    sketch_document(doc, rih, num_hashes, min_hash);

    std::ofstream of(mh_cache.string());
    of.write(reinterpret_cast<const char*>(min_hash->hashes.data()),
             min_hash->hashes.size() * sizeof(min_hash->hashes[0]));
}

void cluster_documents_kmeans(std::vector<Sketch>& min_hashes, size_t num_hashes) {
    size_t num_documents = min_hashes.size();
    size_t points_per_centroid = 20;
    size_t num_centroids = num_documents / points_per_centroid;
    std::vector<Sketch> centroids;
    centroids.reserve(num_centroids);

    size_t j = 0;
    for (size_t i = 0; i < num_centroids; ++i) {
        CollectSketch cmh;
        for (size_t k = 0; k < points_per_centroid && j < min_hashes.size(); ++k, ++j) {
            cmh.add(min_hashes[j]);
        }
        // centroids.emplace_back(cmh.get_complete());
        centroids.emplace_back(cmh.get(num_hashes));
    }

    // for each point, find closest centroid
    std::vector<size_t> assignments;
    assignments.resize(num_documents);

    for (size_t r = 0; r < 20; ++r) {

        double dist_sum = 0;
        for (size_t doc_id = 0; doc_id < num_documents; ++doc_id) {
            const Sketch& doc_hash = min_hashes[doc_id];
            size_t best = 0;
            double best_dist = 1.0 - doc_hash.dist_jaccard(centroids[0]);
            for (size_t j = 1; j < num_centroids; ++j) {
                double dist = 1.0 - doc_hash.dist_jaccard(centroids[j]);
                if (dist < best_dist)
                    best = j, best_dist = dist;
            }
            assignments[doc_id] = best;
            LOG1 << "best " << best << " dist " << best_dist;
            dist_sum += best_dist;
        }

        LOG1 << "dist_sum " << dist_sum;

        // calculate new centroids
        for (size_t i = 0; i < num_centroids; ++i) {
            CollectSketch cmh;
            size_t p = 0;
            for (size_t j = 0; j < assignments.size(); ++j) {
                if (assignments[j] == i) {
                    cmh.add(min_hashes[j]);
                    ++p;
                }
            }
            if (cmh.size() != 0) {
                // centroids[i] = cmh.get_complete();
                centroids[i] = cmh.get(num_hashes);
                sLOG1 << "c" << i << " - " << p;
            }
            else {
                LOG1 << "No points for centroids " << i;
            }
            // LOG1 << centroids[i].hashes;
        }
    }
}

void cluster_documents_ward(const std::vector<Sketch>& min_hashes_, size_t num_hashes) {
    size_t num_documents = min_hashes_.size();

    std::vector<Sketch> min_hashes = min_hashes_;

    std::vector<size_t> parents;
    std::vector<size_t> orig_size;
    parents.resize(num_documents);
    orig_size.resize(num_documents);
    for (size_t i = 0; i < num_documents; ++i) {
        parents[i] = i;
        orig_size[i] = min_hashes_[i].hashes.back();
    }

    using IndexPair = std::pair<size_t, size_t>;
    AddressablePriorityQueue<IndexPair, double, std::greater<double> > apq;

    for (size_t i = 0; i < min_hashes.size(); ++i) {
        for (size_t j = i + 1; j < min_hashes.size(); ++j) {
            double dist = min_hashes[i].dist_density(min_hashes[j], num_hashes);
            apq.insert(IndexPair(i, j), dist);
            LOG1 << "i=" << i << " j=" << j << " dist " << dist << " apq "
                 << apq.top_priority();
        }
    }

    for (size_t r = 0; r + 50 < num_documents; ++r) {
        IndexPair top = apq.top();
        double prio = apq.top_priority();

        LOG1 << "Combine top=" << top << " prio=" << prio;
        apq.pop();

        // combine min hashes
        parents[top.second] = top.first;
        min_hashes[top.first] = min_hashes[top.first].merge(
            min_hashes[top.second], num_hashes);
        min_hashes[top.second].clear();

        // erase pair which is combined
        apq.erase(top);

        // erase all entries with top.second
        for (size_t i = 0; i < top.second; ++i) {
            if (parents[i] == i) {
                apq.erase(IndexPair(i, top.second));
            }
        }
        for (size_t j = top.second + 1; j < num_documents; ++j) {
            if (parents[j] == j) {
                apq.erase(IndexPair(top.second, j));
            }
        }

        // update distances
        for (size_t i = 0; i < top.first; ++i) {
            if (parents[i] != i)
                continue;

            double dist = min_hashes[i].dist_density(
                min_hashes[top.first], num_hashes);
            apq.insert(IndexPair(i, top.first), dist);
            LOG1 << "update i=" << i << " j=" << top.first
                 << " dist " << dist << " apq "
                 << apq.top_priority();
        }
        for (size_t j = top.first + 1; j < num_documents; ++j) {
            if (parents[j] != j)
                continue;

            double dist = min_hashes[top.first].dist_density(
                min_hashes[j], num_hashes);
            apq.insert(IndexPair(top.first, j), dist);
            LOG1 << "update i=" << top.first << " j=" << j
                 << " dist " << dist << " apq "
                 << apq.top_priority();
        }
    }

    std::ofstream of("cluster.txt");
    of << "digraph a {\n";
    for (size_t i = 0; i < num_documents; ++i) {
        std::string label = std::to_string(i) + " / " + std::to_string(orig_size[i]);
        if (parents[i] == i)
            label += " / " + std::to_string(min_hashes[i].hashes.back());
        of << i << "[label=" << std::quoted(label) << "];\n";
        of << i << " -> " << parents[i] << ";\n";
    }
    of << "}\n";
}

void construct(const fs::path& in_dir, const fs::path& out_dir) {
    timer t;

    std::vector<fs::path> sorted_paths;
    get_sorted_file_names(
        in_dir, &sorted_paths,
        [](const fs::path& p) { return p.extension() == ".ctx"; });

    if (0)
    {
        std::vector<fs::path> paths = sorted_paths;
        // std::random_shuffle(paths.begin(), paths.end());
        if (paths.size() > 1024)
            paths.resize(1024);
        sorted_paths = paths;
    }

    std::cout << "Construct index from "
              << sorted_paths.size() << " documents." << std::endl;

    file::ranfold_index_header rih;

    rih.m_term_space = 2 * 1024 * 1024;
    rih.m_term_hashes = 1;
    rih.m_doc_space_bytes = (16 * 1024 + 7) / 8;
    rih.m_doc_hashes = 2;

    for (const auto& p : sorted_paths) {
        if (p.extension() != ".ctx")
            continue;
        rih.m_file_names.push_back(p.filename());
    }

    static const size_t num_hashes = 1024 * 16;
    std::vector<Sketch> min_hashes;
    document<31> doc;

    for (const auto& p : sorted_paths) {
        Sketch min_hash;
        sketch_path(p, doc, rih, num_hashes, &min_hash);
        min_hashes.emplace_back(min_hash);
    }

    cluster_documents_ward(min_hashes, num_hashes);
}

void construct_random(const fs::path& out_file,
                      uint64_t term_space, uint64_t num_term_hashes,
                      uint64_t doc_space_bits, uint64_t num_doc_hashes,
                      uint64_t num_documents, size_t document_size,
                      size_t seed) {
    timer t;

    file::ranfold_index_header rih;
    rih.m_file_names.reserve(num_documents);
    for (size_t i = 0; i < num_documents; ++i) {
        rih.m_file_names.push_back("file_" + std::to_string(i));
    }

    // compress factor 2
    doc_space_bits = num_documents / 2;

    rih.m_term_space = term_space;
    rih.m_term_hashes = num_term_hashes;
    rih.m_doc_space_bytes = (doc_space_bits + 7) / 8;
    rih.m_doc_hashes = num_doc_hashes;

    doc_space_bits = rih.m_doc_space_bytes * 8;

    {
        std::mt19937 rng(seed);

        static const size_t num_hashes = 1024;
        std::vector<Sketch> min_hashes;
        document<31> doc;

        for (size_t doc_id = 0; doc_id < num_documents; ++doc_id) {
            document_generator(doc, document_size, rng);
            Sketch mh;
            sketch_document(doc, rih, num_hashes, &mh);
            min_hashes.emplace_back(mh);
        }

        cluster_documents_ward(min_hashes, num_hashes);
    }

    std::vector<uint8_t> data;
    data.resize(rih.m_doc_space_bytes * rih.m_term_space);

    std::mt19937 rng(seed);

    t.active("generate");

    std::vector<document<31> > docset;
    document<31> doc;

    for (size_t doc_id = 0; doc_id < num_documents / 2; ++doc_id) {

        while (docset.size() < 2) {
            docset.push_back(document_generator(doc, document_size, rng));
        }

        // put two documents into one index
        mark_document(docset[0], rih, data, doc_id / 2);
        mark_document(docset[1], rih, data, doc_id / 2);

        docset.clear();
    }

    size_t bit_count = tlx::popcount(data.data(), data.size());
    LOG1 << "ratio of ones: "
         << static_cast<double>(bit_count) / (data.size() * 8);

    t.active("write");
    rih.write_file(out_file, data);
    t.stop();

    std::cout << t;
}

} // namespace cobs::ranfold_index

/******************************************************************************/

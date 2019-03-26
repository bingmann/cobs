/*******************************************************************************
 * cobs/util/filelist.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_FILELIST_HEADER
#define COBS_UTIL_FILELIST_HEADER

#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>

#include <algorithm>
#include <iomanip>

namespace cobs {

enum class FileType {
    Any,
    Document,
};

/*!
 * FileList is used to scan a directory, filter the files for specific types,
 * and then run batch processing methods on them.
 */
class FileList
{
public:
    //! accept a file list, sort by name
    FileList(const std::vector<fs::path>& paths)
        : paths_(paths) {
        std::sort(paths_.begin(), paths_.end());
    }

    //! read a directory, filter files
    FileList(const fs::path& dir, FileType filter = FileType::Any) {
        fs::recursive_directory_iterator it(dir), end;
        while (it != end) {
            if (accept(*it, filter)) {
                paths_.emplace_back(*it);
            }
            ++it;
        }
        std::sort(paths_.begin(), paths_.end());
    }

    //! filter method to match file specifications
    bool accept(const fs::path& path, FileType filter) {
        switch (filter) {
        case FileType::Any:
            return true;
        case FileType::Document:
            return path.extension() == ".ctx" ||
                   path.extension() == ".fasta" ||
                   path.extension() == ".cobs_doc";
        }
        return false;
    }

    //! return list of paths
    std::vector<fs::path> paths() const { return paths_; }

    //! sort files by size
    void sort_by_size() {
        std::sort(paths_.begin(), paths_.end(),
                  [](const fs::path& p1, const fs::path& p2) {
                      auto s1 = fs::file_size(p1), s2 = fs::file_size(p2);
                      std::string f1 = p1.string(), f2 = p2.string();
                      return std::tie(s1, f1) < std::tie(s2, f2);
                  });
    }

    //! process each file
    void process_each(void (*func)(const fs::path&)) const {
        for (size_t i = 0; i < paths_.size(); i++) {
            func(paths_[i]);
        }
    }

    //! process files in batch with output file generation
    template <typename Functor>
    void process_batches(size_t batch_size, Functor func) const {
        std::string first_filename, last_filename;
        size_t j = 1;
        std::vector<fs::path> batch;
        for (size_t i = 0; i < paths_.size(); i++) {
            std::string filename = cobs::base_name(paths_[i]);
            if (first_filename.empty()) {
                first_filename = filename;
            }
            last_filename = filename;
            batch.push_back(paths_[i]);

            if (batch.size() == batch_size ||
                (!batch.empty() && i + 1 == paths_.size()))
            {
                std::string out_file = "[" + first_filename + "-" + last_filename + "]";
                std::cout << "IN"
                          << " - " << std::setfill('0') << std::setw(7) << j
                          << " - " << out_file << std::endl;

                func(batch, out_file);

                std::cout << "OK"
                          << " - " << std::setfill('0') << std::setw(7) << j
                          << " - " << out_file << std::endl;
                batch.clear();
                first_filename.clear();
                j++;
            }
        }
    }

private:
    std::vector<fs::path> paths_;
};

} // namespace cobs

#endif // !COBS_UTIL_FILELIST_HEADER

/******************************************************************************/

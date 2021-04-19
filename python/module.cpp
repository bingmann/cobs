/*******************************************************************************
 * python/module.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include <cobs/construction/classic_index.hpp>
#include <cobs/construction/compact_index.hpp>
#include <cobs/file/classic_index_header.hpp>
#include <cobs/file/compact_index_header.hpp>
#include <cobs/query/classic_search.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/calc_signature_size.hpp>

#include <cobs/settings.hpp>

#include <tlx/string.hpp>

/******************************************************************************/

uint64_t calc_signature_size(
        uint64_t num_elements, double num_hashes,
        double false_positive_rate)
{
    return cobs::calc_signature_size(num_elements, num_hashes, false_positive_rate);
}

void classic_construct(
    const std::string& input, const std::string& out_file,
    const cobs::ClassicIndexParameters& index_params,
    std::string file_type, std::string tmp_path)
{
    // read file list
    cobs::DocumentList filelist(input, cobs::StringToFileType(file_type));
    cobs::classic_construct(filelist, out_file, tmp_path, index_params);
}

void classic_construct_list(
    const cobs::DocumentList& list, const std::string& out_file,
    const cobs::ClassicIndexParameters& index_params,
    std::string tmp_path)
{
    cobs::classic_construct(list, out_file, tmp_path, index_params);
}

void classic_construct_from_documents(
        const cobs::DocumentList& list, const std::string& out_dir,
        const cobs::ClassicIndexParameters& index_params)
{
    cobs::classic_construct_from_documents(list, out_dir, index_params);
}

/******************************************************************************/

void compact_construct(
    const std::string& input, const std::string& out_file,
    const cobs::CompactIndexParameters& index_params,
    std::string file_type, std::string tmp_path)
{
    // read file list
    cobs::DocumentList filelist(input, cobs::StringToFileType(file_type));
    cobs::compact_construct(filelist, out_file, tmp_path, index_params);
}

void compact_construct_list(
    const cobs::DocumentList& list, const std::string& out_file,
    const cobs::CompactIndexParameters& index_params,
    std::string tmp_path)
{
    cobs::compact_construct(list, out_file, tmp_path, index_params);
}

/******************************************************************************/

int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;

PYBIND11_MODULE(cobs_index, m) {
    m.doc() = R"pbdoc(
        COBS Python API Reference
        -------------------------
        .. currentmodule:: cobs_index

        .. rubric:: Classes and Types
        .. autosummary::
           :toctree: _generated

           FileType
           DocumentEntry
           DocumentList
           ClassicIndexParameters
           CompactIndexParameters
           Search

        .. rubric:: Methods

        .. autosummary::
           :toctree: _generated

           calc_signature_size
           classic_construct
           classic_construct_list
           compact_construct
           compact_construct_list
           disable_cache
    )pbdoc";

    m.def("disable_cache",
          [](bool disable) {
              cobs::gopt_disable_cache = disable;
          },
          "disable FastA/FastQ cache files globally",
          py::arg("disable") = true);

    /**************************************************************************/
    // DocumentList

    using cobs::FileType;
    py::enum_<FileType>(
        m, "FileType", py::arithmetic(), "Enum of indexable file types")
    .value("Any", FileType::Any, "Accept any supported file types in dir scan")
    .value("Text", FileType::Text,
           "Text file: read as sequential symbol stream")
    .value("Cortex", FileType::Cortex, "McCortex file")
    .value("KMerBuffer", FileType::KMerBuffer,
           "Explicit list of k-mers for testing")
    .value("Fasta", FileType::Fasta,
           "FastA file, parsed as one document containing subsequences")
    .value("Fastq", FileType::Fastq,
           "FastQ file, parsed as one document containing subsequences")
    .value("FastaMulti", FileType::FastaMulti,
           "FastA multi-file, parsed as multiple documents")
    .value("FastqMulti", FileType::FastqMulti,
           "FastQ multi-file, parsed as multiple documents")
    .export_values();

    using cobs::DocumentEntry;
    py::class_<DocumentEntry>(
        m, "DocumentEntry",
        "DocumentEntry object specifying a document to be indexed.")
    .def_readwrite("path", &DocumentEntry::path_,
                   "file system path to document")
    .def_readwrite("type", &DocumentEntry::type_,
                   "type of document")
    .def_readwrite("name", &DocumentEntry::name_,
                   "name of the document")
    .def_readwrite("size", &DocumentEntry::size_,
                   "size of the document in bytes")
    .def_readwrite("subdoc_index", &DocumentEntry::subdoc_index_,
                   "subdocument index (for Multifile FASTA, FASTQ, etc)")
    .def_readwrite("term_size", &DocumentEntry::term_size_,
                   "fixed term (term) size or zero")
    .def_readwrite("term_count", &DocumentEntry::term_count_,
                   "number of terms if fixed size")
    .def("num_terms",
         [](DocumentEntry& e, const size_t k) {
             return e.num_terms(k);
         },
         "number of terms",
         py::arg("k"));

    using cobs::DocumentList;
    py::class_<DocumentList>(
        m, "DocumentList",
        "List of DocumentEntry objects for indexing")
    .def(py::init<>(),
         "default constructor, constructs an empty list.")
    .def(py::init<std::string, FileType>(),
         "construct and add path recursively.",
         py::arg("root"),
         py::arg("filter") = FileType::Any)
    .def("size", &DocumentList::size,
         "return number of DocumentEntry in list")
    .def("add",
         [](DocumentList& l, const std::string& path) {
             return l.add(path);
         },
         "identify and add new file to DocumentList",
         py::arg("path"))
    .def("add_recursive",
         [](DocumentList& l, const std::string& path, FileType filter) {
             return l.add_recursive(path, filter);
         },
         "identify and add new file to DocumentList",
         py::arg("path"),
         py::arg("filter") = FileType::Any)
    .def("sort_by_path", &DocumentList::sort_by_path,
         "sort entries by path")
    .def("sort_by_size", &DocumentList::sort_by_size,
         "sort entries by size")
    // Bare bones interface
    .def("__len__", &DocumentList::size)
    .def("__getitem__",
         [](const DocumentList& l, size_t i) { return l[i]; })
    .def("__iter__",
         [](const DocumentList& l) {
             return py::make_iterator(l.list().begin(), l.list().end());
         },
         // essential: keep object alive while iterator exists
         py::keep_alive<0, 1>());

    /**************************************************************************/
    // ClassicIndexParameters

    using cobs::ClassicIndexParameters;
    py::class_<ClassicIndexParameters>(
        m, "ClassicIndexParameters",
        "Parameter object for classic_construct()")
    .def(py::init<>(),
         "constructor, fills the object with default parameters.")
    .def_readwrite(
        "term_size", &ClassicIndexParameters::term_size,
        "length of terms / k-mers, default 31")
    .def_readwrite(
        "canonicalize", &ClassicIndexParameters::canonicalize,
        "canonicalization flag for base pairs, default true")
    .def_readwrite(
        "num_hashes", &ClassicIndexParameters::num_hashes,
        "number of hash functions, provided by user, default 1")
    .def_readwrite(
        "false_positive_rate", &ClassicIndexParameters::false_positive_rate,
        "false positive rate, provided by user, default 0.3")
    .def_readwrite(
        "signature_size", &ClassicIndexParameters::signature_size,
        "signature size, either provided by user or calculated "
        "from false_positive_rate if zero, default 0")
    .def_readwrite(
        "mem_bytes", &ClassicIndexParameters::mem_bytes,
        "memory to use bytes to create index, default 80% of RAM")
    .def_readwrite(
        "num_threads", &ClassicIndexParameters::num_threads,
        "number of threads to use, default all cores")
    .def_readwrite(
        "log_prefix", &ClassicIndexParameters::log_prefix,
        "log prefix (used by compact index construction), default empty")
    .def_readwrite(
        "clobber", &ClassicIndexParameters::clobber,
        "clobber erase output directory if it exists, default false")
    .def_readwrite(
        "continue", &ClassicIndexParameters::continue_,
        "continue in existing output directory, default false")
    .def_readwrite(
        "keep_temporary", &ClassicIndexParameters::keep_temporary,
        "keep temporary files during construction, default false");

    /**************************************************************************/
    // calc_signature_size()

    m.def(
            "calc_signature_size", &calc_signature_size, R"pbdoc(

Calculate the number of cells in a Bloom filter with k hash functions into which num_elements are inserted such that it has expected given fpr.

:param uint64_t num_elements:
:param double num_hashes:
:param double false_positive_rate:

        )pbdoc",
            py::arg("num_elements"),
            py::arg("num_hashes"),
            py::arg("false_positive_rate"));

    /**************************************************************************/
    // classic_construct()

    m.def(
        "classic_construct", &classic_construct, R"pbdoc(

Construct a COBS Classic Index from a path of input files.

:param str input: path to the input directory or file
:param str out_file: path to the output ``.cobs_classic`` index file
:param ClassicIndexParameters index_params: instance of classic index parameter object
:param str file_type: filter input documents by file type (any, text, cortex, fasta, etc), default: any
:param str tmp_path: directory for intermediate index files, default: ``out_file`` + ``.tmp``

        )pbdoc",
        py::arg("input"),
        py::arg("out_file"),
        py::arg("index_params") = ClassicIndexParameters(),
        py::arg("file_type") = "any",
        py::arg("tmp_path") = "");

    m.def(
        "classic_construct_list", &classic_construct_list, R"pbdoc(

Construct a COBS Classic Index from a pre-populated DocumentList object.

:param DocumentList input: DocumentList object of documents to index
:param str out_file: path to the output ``.cobs_classic`` index file
:param ClassicIndexParameters index_params: instance of classic index parameter object
:param str tmp_path: directory for intermediate index files, default: ``out_file`` + ``.tmp``

        )pbdoc",
        py::arg("list"),
        py::arg("out_file"),
        py::arg("index_params") = ClassicIndexParameters(),
        py::arg("tmp_path") = "");

    m.def(
            "classic_construct_from_documents", &classic_construct_from_documents, R"pbdoc(

Construct a COBS Classic Index from a pre-populated DocumentList object.

:param DocumentList input: DocumentList object of documents to index
:param str out_dir: path to the output directory
:param ClassicIndexParameters index_params: instance of classic index parameter object

        )pbdoc",
            py::arg("list"),
            py::arg("out_dir"),
            py::arg("index_params") = ClassicIndexParameters());

    /**************************************************************************/
    // CompactIndexParameters

    using cobs::CompactIndexParameters;
    py::class_<CompactIndexParameters>(
        m, "CompactIndexParameters",
        "Parameter object for compact_construct()")
    .def(py::init<>(),
         "constructor, fills the object with default parameters.")
    .def_readwrite(
        "term_size", &CompactIndexParameters::term_size,
        "length of terms / k-mers, default 31")
    .def_readwrite(
        "canonicalize", &CompactIndexParameters::canonicalize,
        "canonicalization flag for base pairs, default true")
    .def_readwrite(
        "num_hashes", &CompactIndexParameters::num_hashes,
        "number of hash functions, provided by user, default 1")
    .def_readwrite(
        "false_positive_rate", &CompactIndexParameters::false_positive_rate,
        "false positive rate, provided by user, default 0.3")
    .def_readwrite(
        "page_size", &CompactIndexParameters::page_size,
        "page size (number of documents grouped into a classic index with "
        "identical parameters), either provided by user or calculated "
        "from number of documents if zero, default 0")
    .def_readwrite(
        "mem_bytes", &CompactIndexParameters::mem_bytes,
        "memory to use bytes to create index, default 80% of RAM")
    .def_readwrite(
        "num_threads", &CompactIndexParameters::num_threads,
        "number of threads to use, default all cores")
    .def_readwrite(
        "clobber", &CompactIndexParameters::clobber,
        "clobber erase output directory if it exists, default false")
    .def_readwrite(
        "continue", &CompactIndexParameters::continue_,
        "continue in existing output directory, default false")
    .def_readwrite(
        "keep_temporary", &CompactIndexParameters::keep_temporary,
        "keep temporary files during construction, default false");

    /**************************************************************************/
    // compact_construct()

    m.def(
        "compact_construct", &compact_construct, R"pbdoc(

Construct a COBS Compact Index from a path of input files.

:param str input: path to the input directory or file
:param str out_file: path to the output ``.cobs_compact`` index file
:param CompactIndexParameters index_params: instance of compact index parameter object
:param str file_type: filter input documents by file type (any, text, cortex, fasta, etc), default: any
:param str tmp_path: directory for intermediate index files, default: ``out_file`` + ``.tmp``

        )pbdoc",
        py::arg("input"),
        py::arg("out_file"),
        py::arg("index_params") = CompactIndexParameters(),
        py::arg("file_type") = "any",
        py::arg("tmp_path") = "");

    m.def(
        "compact_construct_list", &compact_construct_list, R"pbdoc(

Construct a COBS Compact Index from a pre-populated DocumentList object.

:param DocumentList input: DocumentList object of documents to index
:param str out_file: path to the output ``.cobs_compact`` index file
:param CompactIndexParameters index_params: instance of compact index parameter object
:param str tmp_path: directory for intermediate index files, default: ``out_file`` + ``.tmp``

        )pbdoc",
        py::arg("list"),
        py::arg("out_file"),
        py::arg("index_params") = CompactIndexParameters(),
        py::arg("tmp_path") = "");

    /**************************************************************************/
    // SearchResult

    using cobs::SearchResult;
    py::class_<SearchResult>(
        m, "SearchResult",
        "Return objects for Search")
    .def(py::init<>(),
         "constructor, fills the object with default parameters.")
    .def_readwrite(
        "doc_name", &SearchResult::doc_name,
        "document name string")
    .def_readwrite(
        "score", &SearchResult::score,
        "score of document");

    /**************************************************************************/
    // Search (renamed from cobs::ClassicSearch)

    using Search = cobs::ClassicSearch;
    py::class_<Search>(
        m, "Search",
        "Search object to run queries on COBS indices")
    .def(py::init<std::string>(),
         "constructor, loads the given classic or compact index file.")
    .def(
        "search",
        [](Search& s,
           const std::string& query, double threshold, size_t num_results)
        {
            // lambda to allocate and return vector
            std::vector<cobs::SearchResult> result;
            s.search(query, result, threshold, num_results);
            return result;
        },
        "search index for query returning vector of matches",
        py::arg("query"),
        py::arg("threshold") = 0.0,
        py::arg("num_results") = 0);

    /**************************************************************************/

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}

/******************************************************************************/

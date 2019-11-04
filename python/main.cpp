/*******************************************************************************
 * python/main.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <pybind11/pybind11.h>

#include <cobs/construction/classic_index.hpp>
#include <cobs/file/classic_index_header.hpp>

#include <cobs/settings.hpp>

#include <tlx/string.hpp>

/******************************************************************************/

cobs::FileType StringToFileType(std::string& s) {
    tlx::to_lower(&s);
    if (s == "any" || s == "*")
        return cobs::FileType::Any;
    if (s == "text" || s == "txt")
        return cobs::FileType::Text;
    if (s == "cortex" || s == "ctx")
        return cobs::FileType::Cortex;
    if (s == "cobs" || s == "cobs_doc")
        return cobs::FileType::KMerBuffer;
    if (s == "fasta")
        return cobs::FileType::Fasta;
    if (s == "fastq")
        return cobs::FileType::Fastq;
    die("Unknown file type " << s);
}

/******************************************************************************/

cobs::DocumentList doc_list(const std::string& path, std::string file_type)
{
    cobs::DocumentList filelist(path, StringToFileType(file_type));
    return filelist;
}

/******************************************************************************/

void classic_construct(
    const std::string& input, const std::string& out_file,
    const cobs::ClassicIndexParameters& index_params,
    std::string file_type, std::string tmp_path)
{
    // read file list
    cobs::DocumentList filelist(input, StringToFileType(file_type));
    cobs::classic_construct(filelist, out_file, tmp_path, index_params);
}

/******************************************************************************/

int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;

PYBIND11_MODULE(cobs_index, m) {
    m.doc() = R"pbdoc(
        COBS Python Interface
        ---------------------
        .. currentmodule:: cobs_index
        .. autosummary::
           :toctree: _generate

           FileType
           DocumentEntry
           DocumentList
           doc_list
           ClassicIndexParameters
           classic_construct
           add
           subtract
    )pbdoc";

    /**************************************************************************/
    // doc_list()

    using cobs::FileType;
    py::enum_<FileType>(
        m, "FileType", py::arithmetic(), "Indexable file types")
    .value("Any", FileType::Any)
    .value("Text", FileType::Text)
    .value("Cortex", FileType::Cortex)
    .value("KMerBuffer", FileType::KMerBuffer)
    .value("Fasta", FileType::Fasta)
    .value("Fastq", FileType::Fastq)
    .value("FastaMulti", FileType::FastaMulti)
    .value("FastqMulti", FileType::FastqMulti)
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
                   "number of terms if fixed size");

    using cobs::DocumentList;
    py::class_<DocumentList>(
        m, "DocumentList",
        "List of DocumentEntry objects returned by doc_list()")
    .def("size", &DocumentList::size,
         "return number of DocumentEntry in list")
    .def("add",
         [](const DocumentList& l, const std::string& path) {
             return l.add(path);
         },
         "identify and add new file to DocumentList",
         py::arg("path"))
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

    m.def(
        "doc_list", &doc_list, R"pbdoc(

Read a list of documents and returns them as a DocumentList containing DocumentEntry objects

:param str path: path to documents to list
:param str file_type: filter input documents by file type (any, text, cortex, fasta, etc), default: any
:param int term_size: term size (k-mer size), default: 31

        )pbdoc",
        py::arg("path"),
        py::arg("file_type") = "any");

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
        "canonicalization flag for base pairs, default false")
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
        py::arg("index_params"),
        py::arg("file_type") = "any",
        py::arg("tmp_path") = "");

    /**************************************************************************/
    //

    /**************************************************************************/

    m.def("add", &add, R"pbdoc(
        Add two numbers
        Some other explanation about the add function.
    )pbdoc");

    m.def("subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers
        Some other explanation about the subtract function.
    )pbdoc");

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}

/******************************************************************************/

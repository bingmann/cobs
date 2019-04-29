/*******************************************************************************
 * python/main.cpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <pybind11/pybind11.h>

#include <cobs/construction/classic_index.hpp>

void construct_classic(
    const std::string& input, const std::string& out_file,
    const cobs::ClassicIndexParameters& index_params,
    std::string file_type)
{
    LOG1 << "Classic construction";
    LOG1 << "  input " << input;
    LOG1 << "  out_file " << out_file;
    LOG1 << "  file_type " << file_type;
    LOG1 << "  params.term_size " << index_params.term_size;
    LOG1 << "  params.false_positive_rate " << index_params.false_positive_rate;
}

int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;

PYBIND11_MODULE(cobs, m) {
    m.doc() = R"pbdoc(
        Pybind11 example plugin
        -----------------------
        .. currentmodule:: cobs
        .. autosummary::
           :toctree: _generate
           add
           subtract
    )pbdoc";

    using cobs::ClassicIndexParameters;
    py::class_<ClassicIndexParameters>(m, "ClassicIndexParameters")
    .def(py::init<>())
    .def_readwrite("term_size", &ClassicIndexParameters::term_size)
    .def_readwrite("canonicalize", &ClassicIndexParameters::canonicalize)
    .def_readwrite("num_hashes", &ClassicIndexParameters::num_hashes)
    .def_readwrite("false_positive_rate",
                   &ClassicIndexParameters::false_positive_rate)
    .def_readwrite("signature_size",
                   &ClassicIndexParameters::signature_size)
    .def_readwrite("mem_bytes", &ClassicIndexParameters::mem_bytes)
    .def_readwrite("num_threads", &ClassicIndexParameters::num_threads)
    .def_readwrite("log_prefix", &ClassicIndexParameters::log_prefix);

    m.def("construct_classic", &construct_classic, R"pbdoc(
        Construct a COBS Classic Index from a path of input files.
    )pbdoc",
          py::arg("input"),
          py::arg("out_file"),
          py::arg("index_params"),
          py::arg("file_type") = "any");

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

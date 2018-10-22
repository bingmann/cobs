################################################################################
# CMakeLists.txt
#
# Copyright (c) 2018 Florian Gauger
# Copyright (c) 2018 Timo Bingmann
#
# All rights reserved. Published under the MIT License in the LICENSE file.
################################################################################

cmake_minimum_required(VERSION 3.9.6)
cmake_policy(VERSION 3.9.6)

project(cobs)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set(SOURCE_FILES
        cobs/construction/classic_index.cpp
        cobs/construction/classic_index.hpp
        cobs/construction/compact_index.cpp
        cobs/construction/compact_index.hpp
        cobs/cortex.hpp
        cobs/document.hpp
        cobs/file/classic_index_header.cpp
        cobs/file/classic_index_header.hpp
        cobs/file/compact_index_header.cpp
        cobs/file/compact_index_header.hpp
        cobs/file/document_header.cpp
        cobs/file/document_header.hpp
        cobs/file/file_io_exception.hpp
        cobs/file/frequency_header.cpp
        cobs/file/frequency_header.hpp
        cobs/file/header.hpp
        cobs/frequency.hpp
        cobs/kmer.hpp
        cobs/query/classic_base.hpp
        cobs/query/classic_base.cpp
        cobs/query/classic_index/base.cpp
        cobs/query/classic_index/base.hpp
        cobs/query/classic_index/mmap.cpp
        cobs/query/classic_index/mmap.hpp
        cobs/query/compact_index/base.cpp
        cobs/query/compact_index/base.hpp
        cobs/query/compact_index/mmap.cpp
        cobs/query/compact_index/mmap.hpp
        cobs/util/error_handling.cpp
        cobs/util/error_handling.hpp
        cobs/util/file.cpp
        cobs/util/file.hpp
        cobs/util/misc.cpp
        cobs/util/misc.hpp
        cobs/util/parameters.cpp
        cobs/util/parameters.hpp
        cobs/util/processing.cpp
        cobs/util/processing.hpp
        cobs/util/query.cpp
        cobs/util/query.hpp
        cobs/util/serialization.cpp
        cobs/util/serialization.hpp
        cobs/util/timer.cpp
        cobs/util/timer.hpp
        )

if(NOT DEFINED SIMD)
    message(WARNING "SIMD was not specified use: -DSIMD=<ON/OFF>\nSIMD default value ON is used")
endif()
option(SIMD "use simd instructions" ON)
if(SIMD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
else()
    add_definitions(-DNO_SIMD)
endif()

if(NOT DEFINED OPENMP)
    message(WARNING "OPENMP was not specified use: -DOPENMP=<ON/OFF>\nOPENMP default value ON is used")
endif()
option(OPENMP "use openmp" ON)
if(OPENMP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lgomp")
    add_definitions(-DNO_OPENMP)
endif()

if(LINUX)
    if(NOT DEFINED AIO)
        message(WARNING "AIO was not specified use: -DAIO=<ON/OFF>\nAIO default value ON is used")
    endif()
    option(AIO "use linux aio to access index" ON)
    if(AIO)
        set(SOURCE_FILES ${SOURCE_FILES} cobs/query/compact_index/aio.hpp cobs/query/compact_index/aio.cpp cobs/util/aio.hpp cobs/util/aio.cpp)
    else()
        add_definitions(-DNO_AIO)
    endif()
else()
    add_definitions(-DNO_AIO)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --param asan-globals=0 -W -Wall -Wno-unused-function -std=c++17 -lstdc++fs -march=native")
MESSAGE(STATUS "CMAKE_CXX_FLAGS:" ${CMAKE_CXX_FLAGS})

file(GLOB_RECURSE TEST_FILES
  "${PROJECT_SOURCE_DIR}/test/src/*.hpp" "${PROJECT_SOURCE_DIR}/test/src/*.cpp")
file(GLOB_RECURSE EXECUTABLE_FILES
  "${PROJECT_SOURCE_DIR}/src/*.cpp")

MESSAGE("SOURCE_FILES     ${SOURCE_FILES}")
MESSAGE("TEST_FILES       ${TEST_FILES}")
MESSAGE("EXECUTABLE_FILES ${EXECUTABLE_FILES}")

################################################################################

### use Google Test ###

add_subdirectory(extlib/googletest)

### use xxHash ###

add_subdirectory(extlib/xxhash/cmake_unofficial)

### use Boost filesystem ###

find_package(Boost 1.42.0 REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})

### use TLX ###

add_subdirectory(extlib/tlx)

################################################################################

include_directories(${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/extlib/xxhash)

add_library(cobs_static STATIC ${SOURCE_FILES})

target_link_libraries(cobs_static
  stdc++fs xxhash tlx ${Boost_LIBRARIES})

################################################################################

file(COPY test/resources DESTINATION test)

foreach(target ${EXECUTABLE_FILES})
  get_filename_component(target_name ${target} NAME_WE)
  add_executable(${target_name} ${target})
  target_link_libraries(${target_name} cobs_static)
endforeach(target)

################################################################################

enable_testing()
include(GoogleTest)
add_executable(tests ${TEST_FILES} ${SOURCE_FILES})
target_compile_definitions(tests PRIVATE cobs_test)
target_link_libraries(tests cobs_static gtest gtest_main)
gtest_add_tests(tests "" ${TEST_FILES})

################################################################################

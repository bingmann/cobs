################################################################################
# CMakeLists.txt
#
# Copyright (c) 2018 Florian Gauger
# Copyright (c) 2018 Timo Bingmann
#
# All rights reserved. Published under the MIT License in the LICENSE file.
################################################################################

cmake_minimum_required(VERSION 3.9.2)
cmake_policy(VERSION 3.9.2)

project(cobs)

# Set a default build type if none was specified
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
    STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set(SOURCE_FILES
        cobs/construction/classic_index.cpp
        cobs/construction/classic_index.hpp
        cobs/construction/compact_index.cpp
        cobs/construction/compact_index.hpp
        cobs/construction/ranfold_index.cpp
        cobs/construction/ranfold_index.hpp
        cobs/cortex_file.hpp
        cobs/fasta_multifile.cpp
        cobs/fasta_multifile.hpp
        cobs/file/classic_index_header.cpp
        cobs/file/classic_index_header.hpp
        cobs/file/compact_index_header.cpp
        cobs/file/compact_index_header.hpp
        cobs/file/file_io_exception.hpp
        cobs/file/header.hpp
        cobs/file/kmer_buffer_header.cpp
        cobs/file/kmer_buffer_header.hpp
        cobs/file/ranfold_index_header.cpp
        cobs/file/ranfold_index_header.hpp
        cobs/kmer.hpp
        cobs/kmer_buffer.hpp
        cobs/query/classic_index/base.cpp
        cobs/query/classic_index/base.hpp
        cobs/query/classic_index/mmap.cpp
        cobs/query/classic_index/mmap.hpp
        cobs/query/classic_search.cpp
        cobs/query/classic_search.hpp
        cobs/query/compact_index/base.cpp
        cobs/query/compact_index/base.hpp
        cobs/query/compact_index/mmap.cpp
        cobs/query/compact_index/mmap.hpp
        cobs/query/ranfold_index/base.cpp
        cobs/query/ranfold_index/base.hpp
        cobs/query/ranfold_index/mmap.cpp
        cobs/query/ranfold_index/mmap.hpp
        cobs/query/search.hpp
        cobs/settings.cpp
        cobs/settings.hpp
        cobs/util/calc_signature_size.cpp
        cobs/util/calc_signature_size.hpp
        cobs/util/error_handling.cpp
        cobs/util/error_handling.hpp
        cobs/util/file.hpp
        cobs/util/misc.cpp
        cobs/util/misc.hpp
        cobs/util/parallel_for.hpp
        cobs/util/process_file_batches.hpp
        cobs/util/query.cpp
        cobs/util/query.hpp
        cobs/util/serialization.hpp
        cobs/util/timer.cpp
        cobs/util/timer.hpp
        cobs/util/zip_stream.cpp
        cobs/util/zip_stream.hpp
        cobs/util/zip_stream_fwd.hpp
        )

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -std=c++17 -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")

MESSAGE(STATUS "CMAKE_CXX_FLAGS:" ${CMAKE_CXX_FLAGS})

file(GLOB_RECURSE TEST_FILES
  "${PROJECT_SOURCE_DIR}/tests/*.hpp" "${PROJECT_SOURCE_DIR}/tests/*.cpp")
file(GLOB_RECURSE EXECUTABLE_FILES
  "${PROJECT_SOURCE_DIR}/src/*.cpp")

MESSAGE("SOURCE_FILES     ${SOURCE_FILES}")
MESSAGE("TEST_FILES       ${TEST_FILES}")
MESSAGE("EXECUTABLE_FILES ${EXECUTABLE_FILES}")

# with run-time STL checks
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG")

# with AddressSanitizer
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

# enable ThreadSanitizer
if(OFF)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -pie -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCOBS_HAVE_THREAD_SANITIZER=1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -pie -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCOBS_HAVE_THREAD_SANITIZER=1")
endif()

################################################################################

### find pthreads ###

find_package(Threads REQUIRED)
set(COBS_LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} ${COBS_LINK_LIBRARIES})

### use Google Test ###

add_subdirectory(extlib/googletest)

### use xxHash ###

add_subdirectory(extlib/xxhash/cmake_unofficial)
set(COBS_LINK_LIBRARIES xxhash ${COBS_LINK_LIBRARIES})

### use ZLIB ###

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

set(COBS_LINK_LIBRARIES ${ZLIB_LIBRARIES} ${COBS_LINK_LIBRARIES})

### use Boost filesystem ###

find_package(Boost 1.42.0 COMPONENTS system filesystem)
if(${Boost_FOUND})
  include_directories(${Boost_INCLUDE_DIRS})
  set(COBS_LINK_LIBRARIES ${Boost_LIBRARIES} ${COBS_LINK_LIBRARIES})
endif()

set(COBS_LINK_LIBRARIES stdc++fs ${COBS_LINK_LIBRARIES})

### use TLX ###

add_subdirectory(extlib/tlx)
set(COBS_LINK_LIBRARIES tlx ${COBS_LINK_LIBRARIES})

################################################################################

include_directories(${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/extlib/xxhash)

add_library(cobs_static STATIC ${SOURCE_FILES})

target_link_libraries(cobs_static ${COBS_LINK_LIBRARIES})

################################################################################

file(COPY tests/resources DESTINATION test)

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

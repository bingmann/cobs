cmake_minimum_required(VERSION 3.9.6)
cmake_policy(VERSION 3.9.6)

project(thesis)

if(NOT DEFINED SIMD)
    message(WARNING "SIMD was not specified use: -DSIMD=<ON/OFF>\nSIMD default value ON is used")
endif()
if(NOT DEFINED OPENMP)
    message(WARNING "OPENMP was not specified use: -DOPENMP=<ON/OFF>\nOPENMP default value ON is used")
endif()

option(SIMD "use simd" ON)
if(SIMD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
else()
    add_definitions(-DNO_SIMD)
endif()

option(OPENMP "use openmp" ON)
if(OPENMP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lgomp")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --param asan-globals=0 -W -Wall -Wno-unused-function -std=c++17 -lstdc++fs -march=native")
MESSAGE(STATUS "CMAKE_CXX_FLAGS:" ${CMAKE_CXX_FLAGS})

set(SOURCE_FILES
        isi/construction/classic_index.cpp
        isi/construction/classic_index.hpp
        isi/construction/compact_index.cpp
        isi/construction/compact_index.hpp
        isi/cortex.hpp
        isi/file/classic_index_header.cpp
        isi/file/classic_index_header.hpp
        isi/file/compact_index_header.cpp
        isi/file/compact_index_header.hpp
        isi/file/file_io_exception.hpp
        isi/file/frequency_header.cpp
        isi/file/frequency_header.hpp
        isi/file/header.hpp
        isi/file/sample_header.cpp
        isi/file/sample_header.hpp
        isi/frequency.hpp
        isi/kmer.hpp
        isi/query/base.hpp
        isi/query/classic_index/base.cpp
        isi/query/classic_index/base.hpp
        isi/query/classic_index/mmap.cpp
        isi/query/classic_index/mmap.hpp
        isi/query/compact_index/base.cpp
        isi/query/compact_index/base.hpp
        isi/query/compact_index/mmap.cpp
        isi/query/compact_index/mmap.hpp
        isi/sample.hpp
        isi/util/error_handling.cpp
        isi/util/error_handling.hpp
        isi/util/file.cpp
        isi/util/file.hpp
        isi/util/misc.cpp
        isi/util/misc.hpp
        isi/util/parameters.cpp
        isi/util/parameters.hpp
        isi/util/processing.cpp
        isi/util/processing.hpp
        isi/util/query.cpp
        isi/util/query.hpp
        isi/util/serialization.cpp
        isi/util/serialization.hpp
        isi/util/timer.cpp
        isi/util/timer.hpp
)

if(UNIX AND NOT APPLE)
    set(SOURCE_FILES ${SOURCE_FILES}
            isi/query/compact_index/aio.hpp
            isi/query/compact_index/aio.cpp
            isi/util/aio.hpp
            isi/util/aio.cpp
    )
endif()

file(GLOB_RECURSE TEST_FILES "${PROJECT_SOURCE_DIR}/test/src/*.hpp" "${PROJECT_SOURCE_DIR}/test/src/*.cpp")
file(GLOB_RECURSE EXECUTABLE_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

MESSAGE("SOURCE_FILES     ${SOURCE_FILES}")
MESSAGE("TEST_FILES       ${TEST_FILES}")
MESSAGE("EXECUTABLE_FILES ${EXECUTABLE_FILES}")

include(dependencies/dependency.cmake)
file(COPY test/resources DESTINATION test)
include_directories(${PROJECT_SOURCE_DIR} ${DEPENDENCY_DIR}/xxhash/src ${DEPENDENCY_DIR}/cli11/src/include)

if(libstdc++fs)
    add_library(stdc++fs UNKNOWN IMPORTED)
    set_property(TARGET stdc++fs PROPERTY IMPORTED_LOCATION ${libstdc++fs})
else()
    message(WARNING "libstdc++fs was not specified use: -D libstdc++fs=\"<path_to_libstdc++fs.a>\"")
endif()

set(libs stdc++fs xxhash)

foreach(target ${EXECUTABLE_FILES})
    get_filename_component(target_name ${target} NAME_WE)
    add_executable(${target_name} ${target} ${SOURCE_FILES})
    target_link_libraries(${target_name} ${libs})
endforeach(target)

enable_testing()
include(GoogleTest)
add_executable(tests ${TEST_FILES} ${SOURCE_FILES})
target_compile_definitions(tests PRIVATE isi_test)
target_link_libraries(tests ${libs} gtest gtest_main)
gtest_add_tests(tests "" ${TEST_FILES})


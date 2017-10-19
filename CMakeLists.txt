cmake_minimum_required(VERSION 3.2)
project(thesis)

set(CMAKE_CXX_STANDARD 14)
if (CMAKE_VERSION VERSION_GREATER 3.7)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -fopenmp")

find_package(Boost REQUIRED COMPONENTS system filesystem)
set(Boost_USE_STATIC_LIBS OFF)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

include(dependencies/dependency.cmake)
file(COPY test/resources DESTINATION test)
include_directories(lib/ ${Boost_INCLUDE_DIRS} ${STXXL_INCLUDE_DIRS} ${DEPENDENCY_DIR}/xxhash/src/)

set(HEADER_FILES
        lib/timer.hpp lib/util.hpp lib/bit_sliced_signatures/bss.hpp lib/file/sample_header.hpp lib/file/bss_header.hpp
        lib/file/frequency_header.hpp lib/server/bss/mmap.hpp lib/server/bss/ifs.hpp lib/file/abss_header.hpp
        lib/server/bss/asio.hpp lib/bit_sliced_signatures/abss.hpp lib/server/bss/base.hpp lib/server/abss/base.cpp
        lib/server/util.hpp
        lib/kmer.hpp lib/sample.hpp lib/cortex.hpp lib/frequency.hpp lib/file/util.hpp lib/file/header.hpp
        lib/kmer.tpp lib/sample.tpp lib/cortex.tpp lib/frequency.tpp lib/file/util.tpp lib/file/header.tpp)

set(TEST_FILES
        test/src/cortex.cpp
        test/src/frequency.cpp
        test/src/bss.cpp
        test/src/file.cpp
        test/src/util.hpp
        test/src/server_bss.cpp
        test/src/abss.cpp)

add_library(thesis_lib
        lib/timer.cpp
        lib/bit_sliced_signatures/bss.cpp
        lib/file/bss_header.cpp
        lib/file/sample_header.cpp
        lib/file/frequency_header.cpp
        lib/file/abss_header.cpp
        lib/server/bss/base.cpp
        lib/server/bss/mmap.cpp
        lib/server/bss/ifs.cpp
        lib/server/abss/base.cpp
        )

set(LIBRARIES thesis_lib ${Boost_LIBRARIES} ${STXXL_LIBRARIES} xxhash)

foreach(target cortex frequency bss server dummy_bss performance temp)
    add_executable(${target} src/main_${target}.cpp)
    target_link_libraries(${target} ${LIBRARIES})
endforeach(target)
add_executable(tests ${TEST_FILES} ${HEADER_FILES})
target_link_libraries(tests ${LIBRARIES} gtest gtest_main)

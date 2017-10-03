cmake_minimum_required(VERSION 3.5)
project(thesis)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-lboost_system -L/software/boost/1.62.0/lib")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

find_package(Boost REQUIRED COMPONENTS filesystem)
set(Boost_USE_STATIC_LIBS OFF)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

include(dependencies/dependency.cmake)
file(COPY test/resources DESTINATION test)

set(HEADER_FILES
        lib/timer.hpp lib/helpers.hpp lib/bloom_filter.hpp lib/file/sample_header.hpp lib/file/bloom_filter_header.hpp
        lib/file/frequency_header.hpp
        lib/kmer.hpp lib/sample.hpp lib/cortex.hpp lib/frequency.hpp lib/file/util.hpp lib/file/header.hpp
        lib/kmer.tpp lib/sample.tpp lib/cortex.tpp lib/frequency.tpp lib/file/util.tpp lib/file/header.tpp)

set(TEST_FILES test/src/cortex.cpp test/src/frequency.cpp test/src/bloom_filter.cpp test/src/file.cpp test/src/util.hpp)

include_directories(lib/ ${Boost_INCLUDE_DIRS} ${DEPENDENCY_DIR}/xxhash/src/)
add_library(thesis_lib lib/timer.cpp lib/file/bloom_filter_header.cpp lib/file/sample_header.cpp lib/bloom_filter.cpp
        lib/file/frequency_header.cpp)
set(LIBRARIES thesis_lib ${Boost_LIBRARIES} xxhash)

add_executable(cortex src/main_cortex.cpp)
add_executable(frequency src/main_frequency.cpp)
add_executable(bloom_filter src/main_bloom_filter.cpp)
add_executable(tests ${TEST_FILES} ${HEADER_FILES})

target_link_libraries(cortex ${LIBRARIES})
target_link_libraries(frequency ${LIBRARIES})
target_link_libraries(bloom_filter ${LIBRARIES})
target_link_libraries(tests ${LIBRARIES} gtest gtest_main)

add_executable(performance src/main_performance.cpp)
add_executable(main src/main.cpp)
target_link_libraries(performance ${LIBRARIES})
target_link_libraries(main ${LIBRARIES})

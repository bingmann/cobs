#cmake_minimum_required(VERSION 3.9)
project(thesis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

MESSAGE(STATUS "CMAKE_CXX_COMPILER: " ${CMAKE_CXX_COMPILER})

find_package(Boost COMPONENTS serialization filesystem)
find_package(GTest)
set(Boost_USE_STATIC_LIBS OFF)

if(GTEST_FOUND)
  set(GTEST_INCLUDE_DIRS "/usr/include")
  set(GTEST_BOTH_LIBRARIES "~/gtest_lib/libgtest.a" "~/gtest_lib/libgtest_main.a")
endif()

message(STATUS "BOOST_FOUND: ${Boost_FOUND}")
message(STATUS "GTEST_FOUND: ${GTEST_FOUND}")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lboost_system")
file(COPY test/resources DESTINATION test)

set(XX_HASH xxHash-0.6.2)

set(HEADER_FILES ${XX_HASH}/xxhash
        lib/timer.hpp lib/helpers.hpp lib/file_format/header.hpp lib/file_format/bloom_filter_header.hpp
        lib/kmer.hpp lib/sample.hpp lib/cortex.hpp lib/frequency.hpp lib/bloom_filter.hpp
        lib/kmer.tpp lib/sample.tpp lib/cortex.tpp lib/frequency.tpp lib/bloom_filter.tpp)

include_directories(lib/ ${XX_HASH} ${Boost_INCLUDE_DIRS} ${GTEST_INCLUDE_DIRS})

add_library(thesis_lib lib/timer.cpp lib/file_format/header.cpp lib/file_format/bloom_filter_header.cpp ${XX_HASH}/xxhash.c)

set(LIBRARIES thesis_lib ${Boost_LIBRARIES})

add_executable(cortex src/main_cortex.cpp)
add_executable(frequency src/main_frequency.cpp)
add_executable(bloom_filter src/main_bloom_filter.cpp)
add_executable(tests test/src/cortex.cpp test/src/frequency.cpp test/src/bloom_filter.cpp test/src/util.hpp ${HEADER_FILES})

target_link_libraries(cortex ${LIBRARIES})
target_link_libraries(frequency ${LIBRARIES})
target_link_libraries(bloom_filter ${LIBRARIES})
target_link_libraries(tests ${LIBRARIES} ${GTEST_BOTH_LIBRARIES})


add_executable(performance src/main_performance.cpp)
add_executable(main src/main.cpp)
target_link_libraries(performance ${LIBRARIES})
target_link_libraries(main ${LIBRARIES})

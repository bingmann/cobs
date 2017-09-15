cmake_minimum_required(VERSION 3.8)
project(thesis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.65.0 COMPONENTS serialization filesystem)
find_package(STXXL REQUIRED)

message(STATUS "BOOST_FOUND: ${Boost_FOUND}")
message(STATUS "STXXL_FOUND: ${STXXL_FOUND}")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STXXL_CXX_FLAGS} -lboost_system")
configure_file(.stxxl .stxxl COPYONLY)

set(XX_HASH xxHash-0.6.2)

set(HEADER_FILES ${XX_HASH}/xxhash lib/kmer.hpp lib/kmer.tpp lib/sample.hpp lib/sample.tpp lib/helpers.hpp)

include_directories(lib/ ${XX_HASH} ${Boost_INCLUDE_DIRS} ${STXXL_INCLUDE_DIRS})

set(LIBRARIES ${Boost_LIBRARIES} ${STXXL_LIBRARIES})

add_executable(bin src/main.cpp ${HEADER_FILES})
add_executable(cortex src/main_cortex.cpp)
add_executable(frequency src/main_frequency.cpp)
add_executable(freq src/main_freq.cpp)
target_link_libraries(bin ${LIBRARIES})
target_link_libraries(cortex ${LIBRARIES})
target_link_libraries(frequency ${LIBRARIES})
target_link_libraries(freq ${LIBRARIES})

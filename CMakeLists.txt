cmake_minimum_required(VERSION 3.8)
project(thesis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.65.0 COMPONENTS serialization filesystem)
find_package(STXXL REQUIRED)

message(STATUS "BOOST_FOUND: ${Boost_FOUND}")
message(STATUS "STXXL_FOUND: ${STXXL_FOUND}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STXXL_CXX_FLAGS} -lboost_system")

configure_file(.stxxl .stxxl COPYONLY)

set(XX_HASH xxHash-0.6.2)

set(HEADER_FILES ${XX_HASH}/xxhash src/lib/kmer.hpp src/lib/sample.cpp src/lib/sample.hpp)
set(SOURCE_FILES src/lib/kmer.cpp src/lib/sample.cpp src/lib/sample.hpp src/lib/helpers.hpp)

set(INCLUDE_DIRECTORIES ${XX_HASH})

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

if(STXXL_FOUND)
    include_directories(${STXXL_INCLUDE_DIRS})
endif()
include_directories(${INCLUDE_DIRECTORIES})

add_library(thesis ${HEADER_FILES} ${SOURCE_FILES})
set(LIBRARIES ${thesis} ${Boost_LIBRARIES} ${STXXL_LIBRARIES})

add_executable(convert src/main/main.cpp)
add_executable(frequency src/main/main_frequency.cpp)
add_executable(freq src/main/main_freq.cpp)
target_link_libraries(convert ${LIBRARIES})
target_link_libraries(frequency ${LIBRARIES})
target_link_libraries(freq ${LIBRARIES})

cmake_minimum_required(VERSION 3.8)
project(thesis)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.65.0 COMPONENTS serialization)
find_package(STXXL REQUIRED)

message(STATUS "BOOST_FOUND: ${Boost_FOUND}")
message(STATUS "STXXL_FOUND: ${STXXL_FOUND}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STXXL_CXX_FLAGS}")

configure_file(.stxxl .stxxl COPYONLY)

set(XX_HASH xxHash-0.6.2)

set(SOURCE_FILES main.cpp)
set(HEADER_FILES ${XX_HASH}/xxhash)

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
add_executable(thesis ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries (thesis ${Boost_LIBRARIES})
target_link_libraries(thesis ${STXXL_LIBRARIES})


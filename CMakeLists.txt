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

file(GLOB_RECURSE HEADER_FILES "lib/*.hpp" "lib/*.tpp")
file(GLOB_RECURSE SOURCE_FILES "lib/*.cpp")
file(GLOB_RECURSE TEST_FILES "test/src/*.hpp" "test/src/*.cpp")
file(GLOB_RECURSE EXECUTABLE_FILES "src/*.cpp")

add_library(thesis_lib ${SOURCE_FILES})
set(LIBRARIES thesis_lib ${Boost_LIBRARIES} ${STXXL_LIBRARIES} xxhash)

foreach(target ${EXECUTABLE_FILES})
    get_filename_component(target_name ${target} NAME_WE)
    add_executable(${target_name} ${target})
    target_link_libraries(${target_name} ${LIBRARIES})
endforeach(target)
add_executable(tests ${TEST_FILES} ${HEADER_FILES})
target_link_libraries(tests ${LIBRARIES} gtest gtest_main)

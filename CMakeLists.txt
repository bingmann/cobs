cmake_minimum_required(VERSION 3.2)
project(thesis)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -fopenmp -std=c++17 -lstdc++fs -mavx2 -march=native")

file(GLOB_RECURSE HEADER_FILES "${PROJECT_SOURCE_DIR}/lib/*.hpp" "${PROJECT_SOURCE_DIR}/lib/*.tpp")
file(GLOB_RECURSE SOURCE_FILES "${PROJECT_SOURCE_DIR}/lib/*.cpp")
file(GLOB_RECURSE TEST_FILES "${PROJECT_SOURCE_DIR}/test/src/*.hpp" "${PROJECT_SOURCE_DIR}/test/src/*.cpp")
file(GLOB_RECURSE EXECUTABLE_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

MESSAGE("HEADER_FILES     ${HEADER_FILES}")
MESSAGE("SOURCE_FILES     ${SOURCE_FILES}")
MESSAGE("TEST_FILES       ${TEST_FILES}")
MESSAGE("EXECUTABLE_FILES ${EXECUTABLE_FILES}")

include(dependencies/dependency.cmake)
file(COPY test/resources DESTINATION test)
include_directories(lib/ ${Boost_INCLUDE_DIRS} ${STXXL_INCLUDE_DIRS} ${DEPENDENCY_DIR}/xxhash/src/)

if(libstdc++fs)
    add_library(stdc++fs UNKNOWN IMPORTED)
    set_property(TARGET stdc++fs PROPERTY IMPORTED_LOCATION ${libstdc++fs})
else()
    message(WARNING "libstdc++fs was not specified use: -D libstdc++fs=\"<path_to_libstdc++fs.a>\"")
endif()

add_library(thesis_lib ${SOURCE_FILES})
set(LIBRARIES thesis_lib stdc++fs ${STXXL_LIBRARIES} xxhash docopt)

foreach(target ${EXECUTABLE_FILES})
    get_filename_component(target_name ${target} NAME_WE)
    add_executable(${target_name} ${target})
    target_link_libraries(${target_name} ${LIBRARIES})
endforeach(target)
add_executable(tests ${TEST_FILES} ${HEADER_FILES})
target_link_libraries(tests ${LIBRARIES} gtest gtest_main)

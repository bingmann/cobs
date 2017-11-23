cmake_minimum_required(VERSION 3.9.6)
cmake_policy(VERSION 3.9.6)

project(thesis)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wno-unused-function -fopenmp -std=c++17 -lstdc++fs -mavx2 -march=native")

file(GLOB_RECURSE SOURCE_FILES "${PROJECT_SOURCE_DIR}/isi/*")
file(GLOB_RECURSE TEST_FILES "${PROJECT_SOURCE_DIR}/test/src/*.hpp" "${PROJECT_SOURCE_DIR}/test/src/*.cpp")
file(GLOB_RECURSE EXECUTABLE_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

MESSAGE("SOURCE_FILES     ${SOURCE_FILES}")
MESSAGE("TEST_FILES       ${TEST_FILES}")
MESSAGE("EXECUTABLE_FILES ${EXECUTABLE_FILES}")

include(dependencies/dependency.cmake)
file(COPY test/resources DESTINATION test)
include_directories(${PROJECT_SOURCE_DIR} ${STXXL_INCLUDE_DIRS} ${DEPENDENCY_DIR}/xxhash/src ${DEPENDENCY_DIR}/cli11/src/include)

if(libstdc++fs)
    add_library(stdc++fs UNKNOWN IMPORTED)
    set_property(TARGET stdc++fs PROPERTY IMPORTED_LOCATION ${libstdc++fs})
else()
    message(WARNING "libstdc++fs was not specified use: -D libstdc++fs=\"<path_to_libstdc++fs.a>\"")
endif()

set(libs stdc++fs ${STXXL_LIBRARIES} xxhash)

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


################################################################################
# CMakeLists.txt
#
# Copyright (c) 2018 Florian Gauger
# Copyright (c) 2018-2019 Timo Bingmann
#
# All rights reserved. Published under the MIT License in the LICENSE file.
################################################################################

cmake_minimum_required(VERSION 3.9.2)
cmake_policy(VERSION 3.9.2)

project(cobs)

# prohibit in-source builds
if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif()

# Set a default build type if none was specified
set(COBS_DEFAULT_BUILD_TYPE "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${COBS_DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${COBS_DEFAULT_BUILD_TYPE}" CACHE
    STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

################################################################################
### Options and Switches

# BUILD_SHARED_LIBS is a standard CMake variable, but we declare it here to
# make it prominent in the GUI.
option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)." OFF)

# Override CMAKE_EXECUTABLE_SUFFIX
option(COBS_EXECUTABLE_SUFFIX "Suffix for executables."
  "${CMAKE_EXECUTABLE_SUFFIX}")
if(COBS_EXECUTABLE_SUFFIX)
  set(CMAKE_EXECUTABLE_SUFFIX "${COBS_EXECUTABLE_SUFFIX}")
endif()

option(COBS_USE_GCOV
  "Compile and run tests with gcov for coverage analysis." OFF)

option(COBS_FETCH_BOOST
  "Download and compile a Boost library" OFF)

################################################################################
### Compiler Flags

# variables to collect compile-time definitions, include dirs, and libraries
set(COBS_DEFINITIONS "")
set(COBS_INCLUDE_DIRS "")
set(COBS_LINK_LIBRARIES "")

# enable more warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic")

# use C++17
set(CMAKE_CXX_STANDARD 17)

# enable warnings
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -march=native -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -march=native -fPIC")

# with run-time STL checks
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG")

# with AddressSanitizer
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

# enable ThreadSanitizer
if(OFF)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -pie -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCOBS_HAVE_THREAD_SANITIZER=1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -pie -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCOBS_HAVE_THREAD_SANITIZER=1")
endif()

message(STATUS "COBS CMAKE_CXX_FLAGS:" ${CMAKE_CXX_FLAGS})

###############################################################################
# enable gcov coverage analysis with gcc

if(COBS_USE_GCOV)
  # find programs
  find_program(GENHTML genhtml)
  find_program(LCOV lcov)

  if(NOT LCOV OR NOT GENHTML)
    message(SEND_ERROR "Coverage analysis requires lcov and genhtml programs.")
  else()
    message(STATUS "Found lcov: ${LCOV}")
    message(STATUS "Found genhtml: ${GENHTML}")
  endif()

  # add coverage anaylsis compile and link flags
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov")

  # add cached variable containing parameters for lcov/genhtml
  set(LCOV_FLAGS "" CACHE STRING "parameters for lcov")
  set(GENHTML_FLAGS --legend --no-branch-coverage
    CACHE STRING "parameters for genhtml")

  # custom target to run before tests
  add_custom_target(lcov-reset
    COMMAND ${LCOV} -q --directory ${CMAKE_BINARY_DIR} --zerocounters
    COMMENT "Resetting code coverage counters")

  # custom lcov target to run tests
  add_custom_target(lcov-runtests
    COMMAND ${CMAKE_CTEST_COMMAND} \${ARGS} || true
    DEPENDS lcov-reset
    COMMENT "Running all unit tests")

  # get git version description
  execute_process(COMMAND git describe --tags
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE GITDESC
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # command sequence to gather, clean and generate HTML coverage report
  add_custom_target(lcov-html
    COMMAND ${LCOV} -q --directory . --capture --output-file lcov.info
    COMMAND ${LCOV} -q --remove lcov.info '/usr/*' '*/extlib/*' ${LCOV_FLAGS} --output-file lcov-clean.info
    COMMAND ${GENHTML} -q -o coverage --title "cobs ${GITDESC}" --prefix ${PROJECT_SOURCE_DIR} ${GENHTML_FLAGS} lcov-clean.info
    DEPENDS lcov-runtests
    COMMENT "Capturing code coverage counters and create HTML coverage report"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # top-level target to run tests and generate coverage report
  add_custom_target(test-coverage
    COMMENT "Generate HTML coverage report "
    DEPENDS lcov-html)

endif(COBS_USE_GCOV)

################################################################################
### Find Required Libraries

### find pthreads ###

find_package(Threads REQUIRED)
set(COBS_LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} ${COBS_LINK_LIBRARIES})
if(CMAKE_USE_PTHREADS_INIT)
  set(COBS_LINK_LIBRARIES pthread ${COBS_LINK_LIBRARIES})
endif()

### use Google Test ###

add_subdirectory(extlib/googletest)

enable_testing()
include(GoogleTest)

### use xxHash ###

add_subdirectory(extlib/xxhash/cmake_unofficial)
set(COBS_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/extlib/xxhash ${COBS_INCLUDE_DIRS})
set(COBS_LINK_LIBRARIES xxhash ${COBS_LINK_LIBRARIES})

### use ZLIB ###

find_package(ZLIB REQUIRED)
set(COBS_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS} ${COBS_INCLUDE_DIRS})
set(COBS_LINK_LIBRARIES ${ZLIB_LIBRARIES} ${COBS_LINK_LIBRARIES})

### use Boost filesystem ###

if(COBS_FETCH_BOOST)

  include(ExternalProject)
  ExternalProject_Add(boost-src
    URL https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.bz2
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/extlib/boost
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )

  set(Boost_BaseDir ${CMAKE_CURRENT_BINARY_DIR}/extlib/boost/src/boost-src/)

  include_directories(SYSTEM ${Boost_BaseDir})

  # glob boost sources
  file(GLOB COBS_MINI_BOOST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/extlib/dummy.cpp
    ${Boost_BaseDir}/libs/system/src/*.cpp
    )

  add_library(cobs_mini_boost STATIC "${COBS_MINI_BOOST_SOURCES}")
  set_target_properties(cobs_mini_boost PROPERTIES LINKER_LANGUAGE CXX)

  set(COBS_BOOST_FOUND ON)
  set(COBS_INCLUDE_DIRS ${Boost_BaseDir} ${COBS_INCLUDE_DIRS})
  set(COBS_LINK_LIBRARIES cobs_mini_boost ${COBS_LINK_LIBRARIES})

else()

  find_package(Boost 1.71.0 COMPONENTS system filesystem)
  if(${Boost_FOUND})
    set(COBS_BOOST_FOUND ON)
    set(COBS_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} ${COBS_INCLUDE_DIRS})
    set(COBS_LINK_LIBRARIES ${Boost_LIBRARIES} ${COBS_LINK_LIBRARIES})
  else()
    message(WARNING "Boost was not found, not building query web server.\n"
      "NOTE:"
      "If you want our cmake script to download and compile Boost, "
      "then add the -DCOBS_FETCH_BOOST=ON flag."
      "\n"
      "Instructions: first add the flag when running cmake, "
      "run make (which will fail, but download Boost), "
      "then rerun cmake on the build directory "
      "(using 'cmake .'), and make again (will work this time). "
      "This is neceesary to correctly build the minimal Boost library.")
  endif()

endif()

set(COBS_LINK_LIBRARIES stdc++fs ${COBS_LINK_LIBRARIES})

### use TLX ###

add_subdirectory(extlib/tlx)
set(COBS_LINK_LIBRARIES tlx ${COBS_LINK_LIBRARIES})

################################################################################
### Descend into Subdirectories

# descend into library source
add_subdirectory(cobs)

# descend into programs
add_subdirectory(src)

# descend into tests
add_subdirectory(tests)

# descend into python
add_subdirectory(python)

################################################################################

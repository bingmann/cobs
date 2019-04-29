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

################################################################################
### Compiler Flags

# variables to collect compile-time definitions, include dirs, and libraries
set(COBS_DEFINITIONS "")
set(COBS_INCLUDE_DIRS "")
set(COBS_LINK_LIBRARIES "")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

# enable more warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic")

# enable warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -std=c++17 -march=native -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")

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

find_package(Boost 1.42.0 COMPONENTS system filesystem)
if(${Boost_FOUND})
  set(COBS_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} ${COBS_INCLUDE_DIRS})
  set(COBS_LINK_LIBRARIES ${Boost_LIBRARIES} ${COBS_LINK_LIBRARIES})
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

cmake_minimum_required(VERSION 2.8.2)

project(gtest-download NONE)

include(ExternalProject)
ExternalProject_Add(gtest
        GIT_REPOSITORY    https://github.com/google/googletest.git
        GIT_TAG           master
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/dependencies/gtest/src"
        BINARY_DIR        "${CMAKE_BINARY_DIR}/dependencies/gtest/build"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND     ""
        INSTALL_COMMAND   ""
        TEST_COMMAND      ""
        )


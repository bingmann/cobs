cmake_minimum_required(VERSION 2.8.2)

project(docopt-download NONE)

include(ExternalProject)
ExternalProject_Add(docopt
        GIT_REPOSITORY    https://github.com/docopt/docopt.cpp.git
        GIT_TAG           v0.6.2
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/dependencies/docopt/src"
        BINARY_DIR        "${CMAKE_BINARY_DIR}/dependencies/docopt/build"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND     ""
        INSTALL_COMMAND   ""
        TEST_COMMAND      ""
        )

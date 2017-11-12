cmake_minimum_required(VERSION 2.8.2)

project(cli11-download NONE)

include(ExternalProject)
ExternalProject_Add(cli11
        GIT_REPOSITORY    https://github.com/CLIUtils/CLI11.git
        GIT_TAG           v1.2.0
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/dependencies/cli11/src"
        BINARY_DIR        "${CMAKE_BINARY_DIR}/dependencies/cli11/build"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND     ""
        INSTALL_COMMAND   ""
        TEST_COMMAND      ""
        )

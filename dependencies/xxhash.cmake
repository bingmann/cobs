cmake_minimum_required(VERSION 2.8.2)

project(xxhash-download NONE)

include(ExternalProject)
ExternalProject_Add(xxhash
        GIT_REPOSITORY    https://github.com/Cyan4973/xxHash.git
        GIT_TAG           master
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/dependencies/xxhash/src"
        BINARY_DIR        ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND     ""
        INSTALL_COMMAND   ""
        TEST_COMMAND      ""
        )

################################################################################
# tests/CMakeLists.txt
#
# Copyright (c) 2019 Timo Bingmann
#
# All rights reserved. Published under the MIT License in the LICENSE file.
################################################################################

# copy resources
file(COPY data DESTINATION .)

# glob tests
file(GLOB_RECURSE TEST_FILES
  "${PROJECT_SOURCE_DIR}/tests/*.hpp" "${PROJECT_SOURCE_DIR}/tests/*.cpp")

# compile into one program
add_executable(cobs_tests ${TEST_FILES} ${SOURCE_FILES})
target_compile_definitions(cobs_tests PRIVATE cobs_test)
target_link_libraries(cobs_tests cobs_static gtest gtest_main)
gtest_add_tests(cobs_tests "" ${TEST_FILES})

################################################################################

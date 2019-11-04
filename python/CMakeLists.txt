################################################################################
# python/CMakeLists.txt
#
# Copyright (c) 2019 Timo Bingmann
#
# All rights reserved. Published under the MIT License in the LICENSE file.
################################################################################

add_subdirectory(pybind11)
pybind11_add_module(cobs_python module.cpp)

# rename cobs_python target output to cobs_index
set_target_properties(cobs_python PROPERTIES OUTPUT_NAME cobs_index)
target_link_libraries(cobs_python PRIVATE cobs_static)

################################################################################

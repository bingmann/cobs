################################################################################
# src/CMakeLists.txt
#
# Copyright (c) 2019 Timo Bingmann
#
# All rights reserved. Published under the MIT License in the LICENSE file.
################################################################################

# glob programs
file(GLOB_RECURSE EXECUTABLE_FILES
  "${PROJECT_SOURCE_DIR}/src/*.cpp")

foreach(target ${EXECUTABLE_FILES})
  get_filename_component(target_name ${target} NAME_WE)
  add_executable(${target_name} ${target})
  target_link_libraries(${target_name} cobs_static)
endforeach(target)

################################################################################

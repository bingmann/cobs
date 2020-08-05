################################################################################
# src/CMakeLists.txt
#
# Copyright (c) 2019-2020 Timo Bingmann
#
# All rights reserved. Published under the MIT License in the LICENSE file.
################################################################################

add_executable(cobs cobs.cpp)
target_link_libraries(cobs cobs_static)

if(COBS_BOOST_FOUND)
  add_executable(cobs_server cobs_server.cpp)
  target_link_libraries(cobs_server cobs_static)
endif()

################################################################################

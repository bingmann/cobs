################################################################################
# cobs/CMakeLists.txt
#
# Copyright (c) 2019 Timo Bingmann
#
# All rights reserved. Published under the MIT License in the LICENSE file.
################################################################################

# glob general sources
file(GLOB COBS_SOURCES
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/*.[ch]pp
  ${CMAKE_CURRENT_SOURCE_DIR}/construction/*.[ch]pp
  ${CMAKE_CURRENT_SOURCE_DIR}/file/*.[ch]pp
  ${CMAKE_CURRENT_SOURCE_DIR}/query/*.[ch]pp
  ${CMAKE_CURRENT_SOURCE_DIR}/query/*/*.[ch]pp
  ${CMAKE_CURRENT_SOURCE_DIR}/util/*.[ch]pp
  )

add_library(cobs_static STATIC ${COBS_SOURCES})
target_compile_definitions(cobs_static PUBLIC ${COBS_DEFINITIONS})
target_include_directories(cobs_static PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(cobs_static SYSTEM PUBLIC ${COBS_INCLUDE_DIRS})
target_link_libraries(cobs_static PUBLIC ${COBS_LINK_LIBRARIES})

################################################################################

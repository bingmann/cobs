set(DEPENDENCY_DIR ${CMAKE_BINARY_DIR}/dependencies)

# Download and unpack googletest at configure time
configure_file(dependencies/gtest.cmake dependencies/gtest/download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${DEPENDENCY_DIR}/gtest/download)
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${DEPENDENCY_DIR}/gtest/download)
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()
add_subdirectory(${DEPENDENCY_DIR}/gtest/src ${DEPENDENCY_DIR}/gtest/build)


configure_file(dependencies/xxhash.cmake dependencies/xxhash/download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${DEPENDENCY_DIR}/xxhash/download)
if(result)
    message(FATAL_ERROR "CMake step for xxhash failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${DEPENDENCY_DIR}/xxhash/download)
if(result)
    message(FATAL_ERROR "Build step for xxhash failed: ${result}")
endif()
add_subdirectory(${DEPENDENCY_DIR}/xxhash/src/cmake_unofficial)

configure_file(dependencies/cli11.cmake dependencies/cli11/download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${DEPENDENCY_DIR}/cli11/download)
if(result)
    message(FATAL_ERROR "CMake step for cli11 failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${DEPENDENCY_DIR}/cli11/download)
if(result)
    message(FATAL_ERROR "Build step for cli11 failed: ${result}")
endif()
add_subdirectory(${DEPENDENCY_DIR}/cli11/src)

configure_file(dependencies/stxxl.cmake dependencies/stxxl/download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${DEPENDENCY_DIR}/stxxl/download)
if(result)
    message(FATAL_ERROR "CMake step for stxxl failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${DEPENDENCY_DIR}/stxxl/download)
if(result)
    message(FATAL_ERROR "Build step for stxxl failed: ${result}")
endif()
add_subdirectory(${DEPENDENCY_DIR}/stxxl/src)


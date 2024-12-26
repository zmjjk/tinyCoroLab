cmake_minimum_required(VERSION 3.22)
project(tinyCoro)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include(GNUInstallDirs)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

set(SRC_INCLUDE_DIR 
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/config")
set(THIRD_PARTY_INCLUDE_DIR
    "${PROJECT_SOURCE_DIR}/third_party/spdlog/include"
    "${PROJECT_SOURCE_DIR}/third_party/googletest/googletest/include"
    "${PROJECT_SOURCE_DIR}/third_party/atomic_queue/include")

option(ENABLE_UNIT_TESTS "Enable unit tests" ON)
message(STATUS "Enable testing: ${ENABLE_UNIT_TESTS}")

find_library(URING_PATH NAMES uring)
if (URING_PATH)
    message(STATUS "Found mylibrary at ${URING_PATH}")
else()
    message(FATAL_ERROR "Could not find mylibrary")
endif()

include_directories(${SRC_INCLUDE_DIR} ${THIRD_PARTY_INCLUDE_DIR})

add_subdirectory(third_party)

add_subdirectory(src)

add_subdirectory(tests)

add_subdirectory(examples)

if(ENABLE_UNIT_TESTS)
  enable_testing()

  add_test(
    NAME base_test
    COMMAND $<TARGET_FILE:base_test>
    )
endif()
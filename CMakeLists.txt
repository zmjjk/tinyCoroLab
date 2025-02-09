cmake_minimum_required(VERSION 3.15)
project(tinycoro
    VERSION 1.0
    LANGUAGES CXX
    DESCRIPTION "C++20 coroutine library combined with io_uring"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# TODO: Complete api visiblity
# set(CMAKE_CXX_VISIBILITY_PRESET hidden) 
# set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

include(GNUInstallDirs)
include(GenerateExportHeader)

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
option(ENABLE_COMPILE_OPTIMIZE "Enable compile options -O3" ON)
# TODO: add more options

message(STATUS "Enable testing: ${ENABLE_UNIT_TESTS}")
message(STATUS "Enable compile options -O3: ${ENABLE_COMPILE_OPTIMIZE}")

find_library(URING_PATH NAMES uring)
if (URING_PATH)
    message(STATUS "Found liburing at ${URING_PATH}")
else()
    message(FATAL_ERROR "Could not find liburing")
endif()

add_subdirectory(third_party)
add_subdirectory(src)
configure_file(${PROJECT_SOURCE_DIR}/config/config.h.in ${PROJECT_SOURCE_DIR}/config/config.h @ONLY)

add_library(${PROJECT_NAME} ${TINYCORO_SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX PREFIX "" VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
if(ENABLE_COMPILE_OPTIMIZE)
  target_compile_options(${PROJECT_NAME} PUBLIC -O3)
endif()
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC ${SRC_INCLUDE_DIR} ${THIRD_PARTY_INCLUDE_DIR})
generate_export_header(${PROJECT_NAME} BASE_NAME CORO EXPORT_FILE_NAME include/coro/export.hpp)

add_subdirectory(tests)
add_subdirectory(examples)

if(ENABLE_UNIT_TESTS)
  enable_testing()

  add_test(
    NAME base_test
    COMMAND $<TARGET_FILE:base_test>
    )
endif()

configure_file(tinycoro.pc.in tinycoro.pc @ONLY)

# TODO: add install command
# install(TARGETS tinycoro)
# install(DIRECTORY include/coro TYPE INCLUDE)
# install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/coro TYPE INCLUDE)
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tinycoro.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
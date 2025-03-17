include(GoogleTest)

file(GLOB_RECURSE TINYCORO_TEST_SOURCES "${PROJECT_SOURCE_DIR}/tests/*test.cpp")

add_custom_target(build-tests COMMAND ${CMAKE_CTEST_COMMAND} --show-only)
add_custom_target(check-tests COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

foreach (tinycoro_test_source ${TINYCORO_TEST_SOURCES})
  get_filename_component(tinycoro_test_filename ${tinycoro_test_source} NAME)
  string(REPLACE ".cpp" "" tinycoro_test_name ${tinycoro_test_filename})
  add_executable(${tinycoro_test_name} EXCLUDE_FROM_ALL ${tinycoro_test_source})
  add_dependencies(build-tests ${tinycoro_test_name})
  add_dependencies(check-tests ${tinycoro_test_name})

  gtest_discover_tests(${tinycoro_test_name}
    EXTRA_ARGS
    --gtest_color=auto
    --gtest_output=xml:${CMAKE_BINARY_DIR}/tests/${tinycoro_test_name}.xml
    --gtest_catch_exceptions=0
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    DISCOVERY_TIMEOUT 120
    PROPERTIES
    TIMEOUT 120
  )

  target_link_libraries(${tinycoro_test_name} ${PROJECT_NAME} gtest)

  # Set test target properties and dependencies.
  set_target_properties(${tinycoro_test_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    COMMAND ${tinycoro_test_name}
  )

  string(REPLACE "_test" "" tinycoro_test_command ${tinycoro_test_name})
  add_custom_target(test-${tinycoro_test_command}
    COMMAND $<TARGET_FILE:${tinycoro_test_name}>
    DEPENDS ${tinycoro_test_name}
    COMMENT "Running ${tinycoro_test_command} tests..."
  )

endforeach ()

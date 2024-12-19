add_executable(base_test "")
target_sources(base_test
  PRIVATE
      base_add.cpp
  )
target_link_libraries(base_test
  PRIVATE
    gtest_main
  )

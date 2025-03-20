file(GLOB SOURCES "*.cpp")

# auto build new examples
foreach(SOURCE ${SOURCES})
    get_filename_component(FILENAME "${SOURCE}" NAME)
    get_filename_component(TARNAME "${SOURCE}" NAME_WE)
    add_executable(${TARNAME} ${FILENAME})
    target_link_libraries(${TARNAME} ${PROJECT_NAME})
    target_compile_definitions(${TARNAME} PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")
    target_compile_definitions(${TARNAME} PRIVATE "$<$<CONFIG:RELEASE>:RELEASE>")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(${TARNAME} PRIVATE "-g")
    endif()
    if(ENABLE_COMPILE_OPTIMIZE)
      target_compile_options(${TARNAME} PUBLIC -O3)
    endif()
endforeach()

add_custom_target(build-debug
  COMMAND echo "build debug..."
  DEPENDS coro_debug
)
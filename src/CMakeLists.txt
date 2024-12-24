add_subdirectory(uring)
add_subdirectory(coro)
add_subdirectory(lock)
add_subdirectory(net)

add_library(${PROJECT_NAME} STATIC ${ALL_OBJECT_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${URING_PATH})
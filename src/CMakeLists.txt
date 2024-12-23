add_subdirectory(uring)
add_subdirectory(coro)
add_subdirectory(lock)
add_subdirectory(net)

add_library(tinyCoro STATIC ${ALL_OBJECT_FILES})
target_link_libraries(tinyCoro PRIVATE ${URING_PATH})
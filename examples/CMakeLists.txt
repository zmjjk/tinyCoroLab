add_executable(mutex mutex.cpp)
target_link_libraries(mutex tinyCoro)
target_compile_options(mutex PRIVATE "-g")

add_executable(tcp_server tcp_server.cpp)
target_link_libraries(tcp_server tinyCoro)

add_executable(uringserver uringserver.cpp)
target_link_libraries(uringserver uring)
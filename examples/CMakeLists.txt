add_executable(mutex mutex.cpp)
target_link_libraries(mutex tinyCoro)
target_compile_options(mutex PRIVATE "-g")
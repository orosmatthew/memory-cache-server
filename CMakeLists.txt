cmake_minimum_required(VERSION 3.22)

project(memory_cache_server)

set(CMAKE_C_STANDARD 11)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

set(SOURCE_FILES src/main.c)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
cmake_minimum_required (VERSION 3.1)
project (phoenix)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
aux_source_directory(src SRC_LIST)
include_directories(include)
add_executable(phoenix_service ${SRC_LIST})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
target_link_libraries(phoenix_service ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(phoenix_service mysqlclient)
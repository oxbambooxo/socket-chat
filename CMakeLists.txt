cmake_minimum_required(VERSION 3.4)
project(gtk3-chat-toy C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(./client)
add_subdirectory(./server)

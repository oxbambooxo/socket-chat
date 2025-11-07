cmake_minimum_required(VERSION 3.5)
project(gtk3-chat-toy-server C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)

include_directories(${LIBUV_INCLUDE_DIRS})
link_directories(${LIBUV_LIBRARY_DIRS})
add_definitions(${LIBUV_CFLAGS_OTHER})

add_executable(server main.c)
target_link_libraries(server PRIVATE ${LIBUV_LIBRARIES})
target_link_libraries(server PRIVATE jansson)

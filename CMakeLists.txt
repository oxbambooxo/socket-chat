project(gtk3-chat-toy C)
cmake_minimum_required(VERSION 3.4)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

include_directories(${JSON_GLIB_INCLUDE_DIRS})
link_directories(${JSON_GLIB_LIBRARY_DIRS})
add_definitions(${JSON_GLIB_CFLAGS_OTHER})

add_definitions(-DCLIENT_GLADE="${CMAKE_SOURCE_DIR}/client/client.glade")

add_executable(client client/main.c)
target_link_libraries(client ${GTK3_LIBRARIES})
target_link_libraries(client ${JSON_GLIB_LIBRARIES})

add_executable(server server/main.c)


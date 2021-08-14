cmake_minimum_required(VERSION 3.4)
project(gtk3-chat-toy-client C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

include_directories(${JSON_GLIB_INCLUDE_DIRS})
link_directories(${JSON_GLIB_LIBRARY_DIRS})
add_definitions(${JSON_GLIB_CFLAGS_OTHER})


if(EXISTS "${CMAKE_SOURCE_DIR}/client-c/client.glade")
    set(CLIENT_GLADE_FILE "${CMAKE_SOURCE_DIR}/client-c/client.glade")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/client.glade")
    set(CLIENT_GLADE_FILE "${CMAKE_SOURCE_DIR}/client.glade")
else()
    message(FATAL_ERROR "not client.glade file finding.")
endif()

add_definitions(-DCLIENT_GLADE="${CLIENT_GLADE_FILE}")

add_executable(client main.c)
target_link_libraries(client ${GTK3_LIBRARIES})
target_link_libraries(client ${JSON_GLIB_LIBRARIES})


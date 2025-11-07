cmake_minimum_required(VERSION 3.14)
project(gtk3-chat-toy C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_VERBOSE_MAKEFILE ON)

include(FetchContent)

FetchContent_Declare(
    jansson
    GIT_REPOSITORY https://github.com/akheron/jansson.git
    GIT_TAG v2.14
)
FetchContent_Declare(
    uthash
    GIT_REPOSITORY https://github.com/troydhanson/uthash.git
    GIT_TAG v2.3.0
)

set(JANSSON_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(JANSSON_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(JANSSON_BUILD_STATIC_LIBS OFF CACHE BOOL "" FORCE)
set(JANSSON_WITHOUT_TESTS ON CACHE BOOL "" FORCE)
set(JANSSON_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(jansson)

FetchContent_GetProperties(uthash)
if(NOT uthash_POPULATED)
    FetchContent_Populate(uthash)
    add_library(uthash INTERFACE)
    target_include_directories(uthash INTERFACE ${uthash_SOURCE_DIR}/src)
endif()

add_subdirectory(./client-c)
add_subdirectory(./server-c)

cmake_minimum_required(VERSION 3.5)
project(gtk3-chat-toy C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_VERBOSE_MAKEFILE ON)

if(CMAKE_VERSION VERSION_LESS 3.11)
    include(ExternalProject)

    set(third_party_root ${CMAKE_BINARY_DIR}/third_party)

    set(JANSSON_INSTALL_DIR ${third_party_root}/jansson/install)
    ExternalProject_Add(jansson_external
        PREFIX ${third_party_root}/jansson
        GIT_REPOSITORY https://github.com/akheron/jansson.git
        GIT_TAG v2.14
        UPDATE_DISCONNECTED 1
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${JANSSON_INSTALL_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DJANSSON_BUILD_DOCS=OFF
            -DJANSSON_BUILD_SHARED_LIBS=OFF
            -DJANSSON_BUILD_STATIC_LIBS=ON
            -DJANSSON_WITHOUT_TESTS=ON
            -DJANSSON_EXAMPLES=OFF
        BUILD_BYPRODUCTS
            ${JANSSON_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}jansson${CMAKE_STATIC_LIBRARY_SUFFIX}
    )

    set(jansson_library_path
        ${JANSSON_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}jansson${CMAKE_STATIC_LIBRARY_SUFFIX})
    add_library(jansson STATIC IMPORTED GLOBAL)
    add_dependencies(jansson jansson_external)
    set_target_properties(jansson PROPERTIES
        IMPORTED_LOCATION ${jansson_library_path}
        INTERFACE_INCLUDE_DIRECTORIES ${JANSSON_INSTALL_DIR}/include)
    if(UNIX AND NOT APPLE)
        set_property(TARGET jansson APPEND PROPERTY INTERFACE_LINK_LIBRARIES m)
    endif()

    ExternalProject_Add(uthash_external
        PREFIX ${third_party_root}/uthash
        GIT_REPOSITORY https://github.com/troydhanson/uthash.git
        GIT_TAG v2.3.0
        UPDATE_DISCONNECTED 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )
    ExternalProject_Get_Property(uthash_external source_dir)
    add_library(uthash INTERFACE)
    add_dependencies(uthash uthash_external)
    target_include_directories(uthash INTERFACE ${source_dir}/src)
else()
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
    set(JANSSON_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(JANSSON_BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(JANSSON_WITHOUT_TESTS ON CACHE BOOL "" FORCE)
    set(JANSSON_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(jansson)

    FetchContent_GetProperties(uthash)
    if(NOT uthash_POPULATED)
        FetchContent_Populate(uthash)
        add_library(uthash INTERFACE)
        target_include_directories(uthash INTERFACE ${uthash_SOURCE_DIR}/src)
    endif()
endif()

add_subdirectory(./client-c)
add_subdirectory(./server-c)

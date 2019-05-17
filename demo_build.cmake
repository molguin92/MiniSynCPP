### DEMO SETUP
include(FetchContent REQUIRED)
include(ExternalProject REQUIRED)

set(MINISYNCPP_DEMO_VERSION_MAJOR "0")
set(MINISYNCPP_DEMO_VERSION_MINOR "5.2")
set(MINISYNCPP_PROTO_VERSION_MAJOR 1)
set(MINISYNCPP_PROTO_VERSION_MINOR 0)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/demo/demo_config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/demo_config.h)

set(PROTOBUF_URL https://github.com/protocolbuffers/protobuf)
set(PROTOBUF_VERSION "3.7.1")

if (APPLE)
    set(PROTOC_ARCHIVE
            "${PROTOBUF_URL}/releases/download/v${PROTOBUF_VERSION}/protoc-${PROTOBUF_VERSION}-osx-x86_64.zip")
elseif (UNIX)
    set(PROTOC_ARCHIVE
            "${PROTOBUF_URL}/releases/download//v${PROTOBUF_VERSION}/protoc-${PROTOBUF_VERSION}-linux-x86_64.zip")
endif ()

# set options ahead of protobuf download
set(protobuf_BUILD_TESTS OFF CACHE BOOL "Turn off Protobuf tests" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "Turn off Protobuf examples" FORCE)
set(protobuf_WITH_ZLIB OFF CACHE BOOL "Turn off Protobuf with zlib" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "Force Protobuf static libs" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "Don't build Protoc" FORCE)

FetchContent_Declare(
        protobuf
        # download
        GIT_REPOSITORY "${PROTOBUF_URL}.git"
        GIT_TAG "v${PROTOBUF_VERSION}"
        # ---
)
FetchContent_GetProperties(protobuf)
if (NOT protobuf_POPULATED)
    message(STATUS "Populating Protobuf sources...")
    FetchContent_Populate(protobuf)
    add_subdirectory("${protobuf_SOURCE_DIR}/cmake" "${protobuf_BINARY_DIR}/cmake")
    message(STATUS "Populating Protobuf sources: done")
endif ()

# add protobuf headers to includes
include_directories(include
        ${protobuf_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include)


link_directories(lib lib/static
        ${CMAKE_CURRENT_BINARY_DIR}/lib
        ${CMAKE_CURRENT_BINARY_DIR}/lib/static)

ExternalProject_Add(
        protoc
        URL ${PROTOC_ARCHIVE}
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/protoc
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        BUILD_ALWAYS 1
        BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/protoc/bin/protoc
)

# generate protobuf definitions
set(PROTOC_PATH ${CMAKE_CURRENT_BINARY_DIR}/protoc/bin/protoc)
set(PROTO_SRC ${CMAKE_CURRENT_BINARY_DIR}/protocol.pb.cc ${CMAKE_CURRENT_BINARY_DIR}/protocol.pb.h)
add_custom_command(OUTPUT ${PROTO_SRC}
        COMMAND ${PROTOC_PATH} protocol.proto --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/src/demo/net/protocol.proto
        DEPENDS protoc
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/demo/net)

add_executable(MiniSyncDemo
        ${CMAKE_CURRENT_BINARY_DIR}/include/demo_config.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/minisync_api.h
        src/demo/main.cpp
        src/demo/node.cpp src/demo/node.h
        src/demo/exception.cpp src/demo/exception.h
        ${PROTO_SRC}
        src/demo/stats.cpp src/demo/stats.h
        include/CLI11/CLI11.hpp
        ${LOGURU_SRC} # loguru, credit to emilk@github
        )

add_dependencies(MiniSyncDemo libprotobuf protoc libminisyncpp_static)

set_target_properties(MiniSyncDemo
        PROPERTIES
        LINK_SEARCH_START_STATIC 1
        LINK_SEARCH_END_STATIC 1
        VERSION "${MINISYNCPP_DEMO_VERSION_MAJOR}.${MINISYNCPP_DEMO_VERSION_MINOR}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/static"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

target_link_libraries(MiniSyncDemo
        libminisyncpp_static # link against the algorithm
        libprotobuf # link against protobuf
        dl ${CMAKE_THREAD_LIBS_INIT})

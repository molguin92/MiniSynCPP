### DEMO SETUP
include(FetchContent REQUIRED)

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
    set(PROTOC_MD5 "2e211695af8062f7f02dfcfa499fc0a7")
elseif (UNIX)
    set(PROTOC_ARCHIVE
            "${PROTOBUF_URL}/releases/download//v${PROTOBUF_VERSION}/protoc-${PROTOBUF_VERSION}-linux-x86_64.zip")
    set(PROTOC_MD5 "8927139bc77e63c32ea017cbea891f46")
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

# get CLI11
set(CLI_TESTING OFF CACHE BOOL "Turn off CLI11 tests" FORCE)
set(CLI_EXAMPLES OFF CACHE BOOL "Turn off CLI11 examples" FORCE)
set(CLI11_URL https://github.com/CLIUtils/CLI11)
set(CLI11_VERSION "1.7.1")
FetchContent_Declare(
        cli11
        # download
        GIT_REPOSITORY "${CLI11_URL}.git"
        GIT_TAG "v${CLI11_VERSION}"
        # ---
)
FetchContent_GetProperties(cli11)
if (NOT cli11_POPULATED)
    message(STATUS "Populating CLI11 sources...")
    FetchContent_Populate(cli11)
    add_subdirectory("${cli11_SOURCE_DIR}" "${cli11_BINARY_DIR}")
    message(STATUS "Populating Protobuf sources: done")
endif ()

include_directories(include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include)


link_directories(lib lib/static
        ${CMAKE_CURRENT_BINARY_DIR}/lib
        ${CMAKE_CURRENT_BINARY_DIR}/lib/static)

FetchContent_Declare(
        protoc
        URL ${PROTOC_ARCHIVE}
        URL_MD5 ${PROTOC_MD5}
)
FetchContent_GetProperties(protoc)
if (NOT protoc_POPULATED)
    message(STATUS "Fetching Protobuf Compiler...")
    FetchContent_Populate(protoc)
    set(PROTOC_PATH ${protoc_SOURCE_DIR}/bin/protoc)
    message(STATUS "Fetching Protobuf Compiler: Done")
endif ()


# generate protobuf definitions
set(PROTO_SRC ${CMAKE_CURRENT_BINARY_DIR}/protocol.pb.cc ${CMAKE_CURRENT_BINARY_DIR}/protocol.pb.h)
add_custom_command(OUTPUT ${PROTO_SRC}
        COMMAND ${PROTOC_PATH} protocol.proto --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/src/demo/net/protocol.proto
        COMMENT "Generating Protobuf definitions..."
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/demo/net)

add_executable(MiniSyncDemo
        ${CMAKE_CURRENT_BINARY_DIR}/include/demo_config.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/minisync_api.h
        src/demo/main.cpp
        src/demo/node.cpp src/demo/node.h
        src/demo/exception.cpp src/demo/exception.h
        src/demo/stats.cpp src/demo/stats.h
        ${PROTO_SRC}
        ${LOGURU_SRC} # loguru, credit to emilk@github
        )

add_dependencies(MiniSyncDemo libprotobuf libminisyncpp_static CLI11)

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
        CLI11 # link against CLI11
        dl ${CMAKE_THREAD_LIBS_INIT})

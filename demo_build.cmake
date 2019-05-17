### DEMO SETUP
set(MINISYNCPP_DEMO_VERSION_MAJOR "0")
set(MINISYNCPP_DEMO_VERSION_MINOR "5.2")
set(MINISYNCPP_PROTO_VERSION_MAJOR 1)
set(MINISYNCPP_PROTO_VERSION_MINOR 0)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/demo/demo_config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/demo_config.h)

set(GITHUB_URL https://github.com/protocolbuffers/protobuf/releases/download)

set(PROTOBUF_VERSION "3.7.1")
set(PROTOBUF_TAR_GZ "${GITHUB_URL}/v${PROTOBUF_VERSION}/protobuf-cpp-${PROTOBUF_VERSION}.tar.gz")
if (APPLE)
    set(PROTOC_ARCHIVE "${GITHUB_URL}/v${PROTOBUF_VERSION}/protoc-${PROTOBUF_VERSION}-osx-x86_64.zip")
elseif (UNIX)
    set(PROTOC_ARCHIVE "${GITHUB_URL}/v${PROTOBUF_VERSION}/protoc-${PROTOBUF_VERSION}-linux-x86_64.zip")
endif ()

ExternalProject_Add(
        protobuf-external
        PREFIX protobuf
        URL ${PROTOBUF_TAR_GZ}
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/protobuf
        CMAKE_CACHE_ARGS
        # "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_BUILD_TYPE:STRING=RELEASE"
        "-Dprotobuf_BUILD_TESTS:BOOL=OFF"
        "-Dprotobuf_BUILD_EXAMPLES:BOOL=OFF"
        "-Dprotobuf_WITH_ZLIB:BOOL=OFF"
        "-DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}"
        "-Dprotobuf_BUILD_SHARED_LIBS:BOOL=OFF"
        "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:STRING=${CMAKE_CURRENT_BINARY_DIR}/protobuf/lib"
        "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:STRING=${CMAKE_CURRENT_BINARY_DIR}/protobuf/lib/static"
        "-Dprotobuf_BUILD_PROTOC_BINARIES:BOOL=OFF"
        # other project specific parameters
        SOURCE_SUBDIR cmake
        BUILD_ALWAYS 1
        STEP_TARGETS build
        INSTALL_COMMAND ""
)

ExternalProject_Get_Property(protobuf-external source_dir)
# add protobuf headers to includes
include_directories(include ${source_dir}/src ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/include)
link_directories(lib lib/static
        ${CMAKE_CURRENT_BINARY_DIR}/lib
        ${CMAKE_CURRENT_BINARY_DIR}/lib/static
        ${CMAKE_CURRENT_BINARY_DIR}/protobuf/lib
        ${CMAKE_CURRENT_BINARY_DIR}/protobuf/lib/static)

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
        include/loguru/loguru.cpp include/loguru/loguru.hpp # loguru, credit to emilk@github
        )

add_dependencies(MiniSyncDemo protobuf-external protoc minisyncpp_static)

set_target_properties(MiniSyncDemo
        PROPERTIES
        LINK_SEARCH_START_STATIC 1
        LINK_SEARCH_END_STATIC 1
        VERSION "${MINISYNCPP_DEMO_VERSION_MAJOR}.${MINISYNCPP_DEMO_VERSION_MINOR}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/static"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

target_link_libraries(MiniSyncDemo
        minisyncpp_static # link against the algorithm
        dl ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_CURRENT_BINARY_DIR}/protobuf/lib/static/libprotobuf.a)
# be very specific with the protobuf lib
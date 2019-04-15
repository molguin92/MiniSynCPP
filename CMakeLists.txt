cmake_minimum_required(VERSION 3.10)
project(MiniSynCPP)
set(CMAKE_CXX_STANDARD 11)
include(ExternalProject REQUIRED)
find_package(Threads REQUIRED)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

set(PROTOBUF_TAR_GZ https://github.com/protocolbuffers/protobuf/releases/download/v3.7.1/protobuf-cpp-3.7.1.tar.gz)
set(PROTO_LIB ${CMAKE_CURRENT_BINARY_DIR}/lib/static/libprotobufd.a)
ExternalProject_Add(
        protobuf-external
        PREFIX protobuf
        URL ${PROTOBUF_TAR_GZ}
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/protobuf
        CMAKE_CACHE_ARGS
        "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
        "-Dprotobuf_BUILD_TESTS:BOOL=OFF"
        "-Dprotobuf_BUILD_EXAMPLES:BOOL=OFF"
        "-Dprotobuf_WITH_ZLIB:BOOL=OFF"
        "-DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}"
        "-Dprotobuf_BUILD_SHARED_LIBS:BOOL=OFF"
        "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:STRING=${CMAKE_CURRENT_BINARY_DIR}/lib"
        "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:STRING=${CMAKE_CURRENT_BINARY_DIR}/lib/static"
        # other project specific parameters
        SOURCE_SUBDIR cmake
        BUILD_ALWAYS 1
        BUILD_BYPRODUCTS ${PROTO_LIB}
        STEP_TARGETS build
        INSTALL_COMMAND ""
)

ExternalProject_Get_Property(protobuf-external source_dir)

include_directories(include ${source_dir}/src ${CMAKE_CURRENT_BINARY_DIR})
link_directories(lib lib/static ${CMAKE_CURRENT_BINARY_DIR})

set(PROTOC_PATH ${CMAKE_CURRENT_BINARY_DIR}/protobuf/protoc)
set(PROTO_SRC ${CMAKE_CURRENT_BINARY_DIR}/protocol.pb.cc ${CMAKE_CURRENT_BINARY_DIR}/protocol.pb.h)

# generate protobuf definitions
add_custom_command(OUTPUT ${PROTO_SRC}
        COMMAND ${PROTOC_PATH} protocol.proto --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS protobuf-external
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/net)

# protobuf_generate_cpp(PROTO_SRC PROTO_HDR src/net/protocol.proto)
add_executable(MiniSynCPP
        src/main.cpp
        src/minisync.cpp src/minisync.h
        src/node.cpp src/node.h
        include/CLI11/CLI11.hpp
        include/loguru/loguru.cpp include/loguru/loguru.hpp # loguru, credit to emilk@github
        src/exception.cpp src/exception.h src/exception.cpp
        ${PROTO_SRC})

add_dependencies(MiniSynCPP protobuf-external)
set_target_properties(MiniSynCPP PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(MiniSynCPP PROPERTIES LINK_SEARCH_END_STATIC 1)

target_link_libraries(MiniSynCPP dl ${CMAKE_THREAD_LIBS_INIT} ${PROTO_LIB})
# be very specific with the protobuf lib

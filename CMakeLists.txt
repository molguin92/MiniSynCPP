cmake_minimum_required(VERSION 3.12)
project(MiniSynCPP)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set(Protobuf_USE_STATIC_LIBS ON) # force static libraries for performance and cross-compiling (to be added later)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

include_directories(include ${PROTOBUF_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
link_directories(lib lib/static)

# generate protobuf definitions
protobuf_generate_cpp(PROTO_SRC PROTO_HDR src/net/protocol.proto)

add_executable(MiniSynCPP
        src/main.cpp
        src/minisync.cpp src/minisync.h
        src/node.cpp src/node.h
        include/CLI11/CLI11.hpp
        include/loguru/loguru.cpp include/loguru/loguru.hpp # loguru, credit to emilk@github
        ${PROTO_SRC} ${PROTO_HDR} src/exception.cpp src/exception.h src/exception.cpp)

set_target_properties(MiniSynCPP PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(MiniSynCPP PROPERTIES LINK_SEARCH_END_STATIC 1)

target_link_libraries(MiniSynCPP
        dl ${CMAKE_THREAD_LIBS_INIT}
        ${Protobuf_LIBRARIES})

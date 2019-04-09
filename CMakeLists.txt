cmake_minimum_required(VERSION 3.13)
project(MiniSynCPP)

find_package(Protobuf REQUIRED)
set(CMAKE_CXX_STANDARD 11)

include_directories(include ${PROTOBUF_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
link_directories(lib lib/static)

# generate protobuf definitions
protobuf_generate_cpp(PROTO_SRC PROTO_HDR src/net/protocol.proto)

add_executable(MiniSynCPP
        src/main.cpp
        src/minisync.cpp src/minisync.h
        src/node.cpp src/node.h
        include/CLI11.hpp
        src/net/protocol.h
        ${PROTO_SRC} ${PROTO_HDR})
target_link_libraries(MiniSynCPP ${Protobuf_LIBRARIES})

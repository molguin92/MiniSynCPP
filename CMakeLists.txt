cmake_minimum_required(VERSION 3.13)
project(MiniSynCPP)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
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
        include/loguru/loguru.cpp include/loguru/loguru.hpp # loguru, credit to emilk@github
        ${PROTO_SRC} ${PROTO_HDR} src/exception.cpp src/exception.h src/exception.cpp)
target_link_libraries(MiniSynCPP
        dl ${CMAKE_THREAD_LIBS_INIT}
        ${Protobuf_LIBRARIES})

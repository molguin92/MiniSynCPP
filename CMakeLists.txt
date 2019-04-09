cmake_minimum_required(VERSION 3.13)
project(MiniSynCPP)

find_package(Protobuf REQUIRED)
set(CMAKE_CXX_STANDARD 11)

include_directories(include ${Protobuf_INCLUDE_DIRS})
link_directories(lib lib/static)

# generate protobuf definitions
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS src/net/protocol.proto)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/protobuf)
file(COPY ${PROTO_SRCS} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/include/protobuf)
file(COPY ${PROTO_HDRS} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/include/protobuf)

add_executable(MiniSynCPP
        src/main.cpp
        src/minisync.cpp src/minisync.h
        src/node.cpp src/node.h
        include/CLI11.hpp
        src/net/protocol.h
        include/protobuf/protocol.pb.cc include/protobuf/protocol.pb.h)
target_link_libraries(MiniSynCPP ${Protobuf_LIBRARIES})

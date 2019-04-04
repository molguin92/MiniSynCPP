cmake_minimum_required(VERSION 3.13)
project(MiniSynCPP)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)
link_directories(lib lib/static)

add_executable(MiniSynCPP src/main.cpp src/minisync.cpp src/minisync.h src/node.cpp src/node.h include/CLI11.hpp)

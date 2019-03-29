cmake_minimum_required(VERSION 3.13)
project(TinySynC__)

set(CMAKE_CXX_STANDARD 11)

add_executable(TinySynC__ main.cpp node.cpp node.h minisync.cpp minisync.h)

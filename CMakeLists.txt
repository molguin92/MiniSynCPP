cmake_minimum_required(VERSION 3.13)
project(MiniSynCPP)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)
link_directories(lib lib/static)

# SocketsCPP as an external dependency
set(SOCKETSCPP_ROOT ${PROJECT_SOURCE_DIR}/src/socketscpp)
ExternalProject_Add(libsocketscpp
        GIT_REPOSITORY git@github.com:molguin92/SocketsCPP.git
        GIT_PROGRESS TRUE
        SOURCE_DIR ${SOCKETSCPP_ROOT}
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${PROJECT_SOURCE_DIR})

add_executable(MiniSynCPP src/main.cpp src/minisync.cpp src/minisync.h src/node.cpp src/node.h include/CLI11.hpp)
add_dependencies(MiniSynCPP libsocketscpp)
target_link_libraries(MiniSynCPP socketscpp)

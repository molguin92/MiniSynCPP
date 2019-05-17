cmake_minimum_required(VERSION 3.11)
project(LibMiniSynCPP CXX)
set(CMAKE_CXX_STANDARD 11)
include(ExternalProject REQUIRED)
find_package(Threads REQUIRED)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

if (UNIX)
    # statically link everything on Linux
    # on OS X we only statically link libminisyncpp and libprotobuf
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif ()

# directories
include_directories(include ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/include)
link_directories(lib lib/static)

### LIBRARY SETUP
# library config
set(LIBMINISYNCPP_BUILD_VERSION "1.0.1")
set(LIBMINISYNCPP_ABI_VERSION "1")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/libminisyncpp/lib_config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/lib_config.h)

# copy the header to the binary directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/libminisyncpp/minisync_api.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/minisync_api.h
        COPYONLY)

## add library for just the algorithm
if (LIBMINISYNCPP_ENABLE_LOGURU)
    set(LIB_SRC
            ${CMAKE_CURRENT_BINARY_DIR}/include/lib_config.h
            ${CMAKE_CURRENT_BINARY_DIR}/include/minisync_api.h
            include/loguru/loguru.cpp include/loguru/loguru.hpp
            src/libminisyncpp/constraints.h src/libminisyncpp/constraints.cpp
            src/libminisyncpp/minisync.h src/libminisyncpp/minisync.cpp
            src/libminisyncpp/minisync_api.h src/libminisyncpp/minisync_api.cpp)
else ()
    set(LIB_SRC
            ${CMAKE_CURRENT_BINARY_DIR}/include/lib_config.h
            ${CMAKE_CURRENT_BINARY_DIR}/include/minisync_api.h
            src/libminisyncpp/constraints.h src/libminisyncpp/constraints.cpp
            src/libminisyncpp/minisync.h src/libminisyncpp/minisync.cpp
            src/libminisyncpp/minisync_api.h src/libminisyncpp/minisync_api.cpp)
endif ()

set(LIB_PROPERTIES
        LINK_SEARCH_START_STATIC 1
        LINK_SEARCH_END_STATIC 1
        VERSION ${LIBMINISYNCPP_BUILD_VERSION}
        SOVERSION ${LIBMINISYNCPP_ABI_VERSION}
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/static"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
        OUTPUT_NAME minisyncpp)

add_library(libminisyncpp_static STATIC ${LIB_SRC})
add_library(libminisyncpp_shared SHARED ${LIB_SRC})
target_link_libraries(libminisyncpp_static dl ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(libminisyncpp_shared dl ${CMAKE_THREAD_LIBS_INIT})

if (LIBMINISYNCPP_ENABLE_LOGURU)
    target_compile_definitions(libminisyncpp_static PUBLIC -DLIBMINISYNCPP_LOGURU_ENABLE)
    target_compile_definitions(libminisyncpp_shared PUBLIC -DLIBMINISYNCPP_LOGURU_ENABLE)
endif ()

set_target_properties(libminisyncpp_static PROPERTIES ${LIB_PROPERTIES})
set_target_properties(libminisyncpp_shared PROPERTIES ${LIB_PROPERTIES})


### Library tests setup
if (LIBMINISYNCPP_BUILD_TESTS)
    add_executable(libminisyncpp_tests
            ${CMAKE_CURRENT_BINARY_DIR}/include/minisync_api.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/tests.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/tests_main.cpp)
    add_dependencies(libminisyncpp_tests libminisyncpp_static)
    target_link_libraries(libminisyncpp_tests libminisyncpp_static dl ${CMAKE_THREAD_LIBS_INIT})

    set_target_properties(libminisyncpp_tests
            PROPERTIES
            LINK_SEARCH_START_STATIC 1
            LINK_SEARCH_END_STATIC 1
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/static"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests"
            OUTPUT_NAME minisyncpp)
endif ()

if (LIBMINISYNCPP_BUILD_DEMO)
    include(demo_build.cmake)
endif ()

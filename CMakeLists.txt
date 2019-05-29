cmake_minimum_required(VERSION 3.11)
project(LibMiniSynCPP CXX)
set(CMAKE_CXX_STANDARD 11)
include(FetchContent REQUIRED)

# show fetchcontent progress
set(FETCHCONTENT_QUIET OFF CACHE BOOL "Turn off FetchContent verbose output." FORCE)

# demo requires loguru
if (LIBMINISYNCPP_BUILD_DEMO)
    set(LIBMINISYNCPP_ENABLE_LOGURU TRUE)
endif ()

if (UNIX)
    # statically link everything on Linux
    # on OS X we only statically link libminisyncpp and libprotobuf
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif ()

# directories
include_directories(include ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/include)
link_directories(lib lib/static)

# dinamically fetch Loguru if required
if (LIBMINISYNCPP_ENABLE_LOGURU)
    set(LOGURU_VERSION "2.0.0")
    set(LOGURU_URL "https://github.com/emilk/loguru")

    FetchContent_Declare(
            loguru
            # download
            GIT_REPOSITORY "${LOGURU_URL}.git"
            GIT_TAG "v${LOGURU_VERSION}"
            # ---
    )
    FetchContent_GetProperties(loguru)
    if (NOT loguru_POPULATED)
        message(STATUS "Populating Loguru sources...")
        FetchContent_Populate(loguru)
        include_directories(${loguru_SOURCE_DIR})
        set(LOGURU_SRC ${loguru_SOURCE_DIR}/loguru.hpp ${loguru_SOURCE_DIR}/loguru.cpp)
        message(STATUS "Populating Loguru sources: done")
    endif ()
endif ()

### Set up reqs for Python bindings
if (LIBMINISYNCPP_WITH_PYTHON)
    message(STATUS "Configuring requirements for Python bindings...")

    set(PYBIND11_VERSION "2.2.4")
    set(PYBIND11_URL "https://github.com/pybind/pybind11")
    set(PYBIND11_CPP_STANDARD -std=c++11)
    set(PYBIND11_PYTHON_VERSION 3.6)

    FIND_PACKAGE(PythonLibs ${PYBIND11_PYTHON_VERSION} REQUIRED)

    FetchContent_Declare(
            pybind11
            # download
            GIT_REPOSITORY "${PYBIND11_URL}.git"
            GIT_TAG "v${PYBIND11_VERSION}"
            # ---
    )
    FetchContent_GetProperties(pybind11)
    if (NOT pybind11_POPULATED)
        message(STATUS "Populating PyBind11 sources...")
        FetchContent_Populate(pybind11)
        include_directories(${PYTHON_INCLUDE_DIRS} ${pybind11_SOURCE_DIR}/include)
        add_subdirectory(${pybind11_SOURCE_DIR})
        message(STATUS "Populating PyBind11 sources: done")
    endif ()
    message(STATUS "Configuring requirements for Python bindings: done")
endif ()

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
# export a global variables for easy use
set(LIBMINISYNCPP_HDR ${CMAKE_CURRENT_BINARY_DIR}/include/minisync_api.h CACHE INTERNAL "libminisyncpp header")
set(LIBMINISYNCPP_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include/ CACHE INTERNAL "libminisyncpp include dir")

## add library for just the algorithm
list(APPEND LIB_SRC
        ${CMAKE_CURRENT_BINARY_DIR}/include/lib_config.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/minisync_api.h
        src/libminisyncpp/constraints.h src/libminisyncpp/constraints.cpp
        src/libminisyncpp/minisync.h src/libminisyncpp/minisync.cpp
        src/libminisyncpp/minisync_api.h src/libminisyncpp/minisync_api.cpp)

if (LIBMINISYNCPP_ENABLE_LOGURU)
    list(APPEND LIB_SRC ${LOGURU_SRC})
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

if (LIBMINISYNCPP_ENABLE_LOGURU)
    find_package(Threads REQUIRED)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    target_link_libraries(libminisyncpp_static dl ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(libminisyncpp_shared dl ${CMAKE_THREAD_LIBS_INIT})
    target_compile_definitions(libminisyncpp_static PUBLIC -DLIBMINISYNCPP_LOGURU_ENABLE)
    target_compile_definitions(libminisyncpp_shared PUBLIC -DLIBMINISYNCPP_LOGURU_ENABLE)
endif ()

set_target_properties(libminisyncpp_static PROPERTIES ${LIB_PROPERTIES})
set_target_properties(libminisyncpp_shared PROPERTIES ${LIB_PROPERTIES})

### build Python bindings
if (LIBMINISYNCPP_WITH_PYTHON)
    pybind11_add_module(pyminisyncpp
            MODULE
            src/python_bindings/pyminisyncpp_gen.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/include/minisync_api.h)
    set_target_properties(pyminisyncpp PROPERTIES
            # LINK_SEARCH_START_STATIC 1
            # LINK_SEARCH_END_STATIC 1
            VERSION ${LIBMINISYNCPP_BUILD_VERSION}
            SOVERSION ${LIBMINISYNCPP_ABI_VERSION}
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/python"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/python"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/python")
    target_link_libraries(pyminisyncpp PRIVATE libminisyncpp_shared)
endif ()

### Library tests setup
if (LIBMINISYNCPP_BUILD_TESTS)
    message(STATUS "Configuring tests...")

    set(CATCH2_VERSION "2.8.0")
    set(CATCH2_URL "https://github.com/catchorg/Catch2")

    FetchContent_Declare(
            catch2
            # download
            GIT_REPOSITORY "${CATCH2_URL}.git"
            GIT_TAG "v${CATCH2_VERSION}"
            # ---
    )
    FetchContent_GetProperties(catch2)
    if (NOT catch2_POPULATED)
        message(STATUS "Populating Catch2 sources...")
        FetchContent_Populate(catch2)
        add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
        message(STATUS "Populating Catch2 sources: done")
    endif ()

    add_executable(libminisyncpp_tests
            ${CMAKE_CURRENT_BINARY_DIR}/include/minisync_api.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/tests.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/tests_main.cpp)

    add_dependencies(libminisyncpp_tests libminisyncpp_static Catch2::Catch2)
    target_link_libraries(libminisyncpp_tests libminisyncpp_static dl ${CMAKE_THREAD_LIBS_INIT} Catch2::Catch2)

    set_target_properties(libminisyncpp_tests
            PROPERTIES
            LINK_SEARCH_START_STATIC 1
            LINK_SEARCH_END_STATIC 1
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/static"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests"
            OUTPUT_NAME minisyncpp)

    message(STATUS "Configuring tests: done")
endif ()

if (LIBMINISYNCPP_BUILD_DEMO)
    include(demo_build.cmake)
endif ()

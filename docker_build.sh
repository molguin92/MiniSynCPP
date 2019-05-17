#!/usr/bin/env bash

printf "Building libminisyncpp in a Docker container...\n"

DOCKER_IMG="minisync_build:latest"

printf "Checking if build image exists in registry: "
if [[ "$(docker images -q ${DOCKER_IMG} 2> /dev/null)" == "" ]]; then
    # check if image exists. If it doesn't, build it.
    printf "not found.\n"
    printf "Building required Docker image:\n"
    docker build --tag=${DOCKER_IMG} - < ./Dockerfile
else
    printf "found.\n"
fi

LINUX_BUILD_DIR="build_x86_64";
RASPI_BUILD_DIR="build_ARMv7";
COMMON_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release \
-UCMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES \
-UCMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES \
-DLIBMINISYNCPP_BUILD_DEMO=TRUE \
-DLIBMINISYNCPP_BUILD_TESTS=TRUE \
-DLIBMINISYNCPP_ENABLE_LOGURU=TRUE \
"

# use the docker image to build both for Linux and for Raspberry Pi
printf "Building MiniSynCPP (x86_64 and ARMv7)...\n"
docker run --name=build --rm -v ${PWD}:/mnt/build ${DOCKER_IMG} /bin/bash -c \
"\
cd /mnt/build;\
mkdir -p ${LINUX_BUILD_DIR};\
cd ${LINUX_BUILD_DIR};\
cmake ${COMMON_CMAKE_FLAGS} .. &&\
cmake --build . --target all -- -j 4;\
./tests/minisyncpp;\
cd ..; chmod 0777 ${LINUX_BUILD_DIR};\
mkdir -p ${RASPI_BUILD_DIR};\
cd ${RASPI_BUILD_DIR};\
cmake -DCMAKE_C_COMPILER=/opt/cross-pi-gcc-8.3.0-2/bin/arm-linux-gnueabihf-gcc \
-DCMAKE_CXX_COMPILER=/opt/cross-pi-gcc-8.3.0-2/bin/arm-linux-gnueabihf-g++ \
-DCMAKE_CXX_FLAGS='-march=armv7-a -mfloat-abi=hard -mfpu=neon-vfpv4' \
${COMMON_CMAKE_FLAGS} .. &&\
cmake --build . --target all -- -j 4;\
cd ..; chmod 0777 ${RASPI_BUILD_DIR};\
"
printf "Done.\n"

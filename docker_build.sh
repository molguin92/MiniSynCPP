#!/usr/bin/env bash

DOCKER_IMG="minisync_build:latest"

if [[ "$(docker images -q ${DOCKER_IMG} 2> /dev/null)" == "" ]]; then
  # check if image exists. If it doesn't, build it.
  docker build --tag=${DOCKER_IMG} - < ./Dockerfile
fi

LINUX_BUILD_DIR="build_x86_64";
RASPI_BUILD_DIR="build_ARMv7";

# use the docker image to build both for Linux and for Raspberry Pi
# Linux:
docker run --name=linux_build --rm -v ${PWD}:/mnt/build ${DOCKER_IMG} /bin/bash -c \
"\
cd /mnt/build;\
mkdir -p ${LINUX_BUILD_DIR};\
cd ${LINUX_BUILD_DIR};\
cmake -DCMAKE_BUILD_TYPE=Release -UCMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES -UCMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES .. &&\
cmake --build . --target all -- -j 4;\
./tests/minisyncpp; \
cd ..; chmod 0777 ${LINUX_BUILD_DIR};
"

# RaspBerry Pi:
docker run --name=raspi_build --rm -v ${PWD}:/mnt/build ${DOCKER_IMG} /bin/bash -c \
"\
cd /mnt/build;\
mkdir -p ${RASPI_BUILD_DIR};\
cd ${RASPI_BUILD_DIR};\
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/opt/cross-pi-gcc-8.3.0-2/bin/arm-linux-gnueabihf-gcc \
-DCMAKE_CXX_COMPILER=/opt/cross-pi-gcc-8.3.0-2/bin/arm-linux-gnueabihf-g++ \
-DCMAKE_CXX_FLAGS='-march=armv7-a -mfloat-abi=hard -mfpu=neon-vfpv4' \
-UCMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES -UCMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES .. &&\
cmake --build . --target all -- -j 4;\
cd ..; chmod 0777 ${RASPI_BUILD_DIR};
"


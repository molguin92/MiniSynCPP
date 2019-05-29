#!/usr/bin/env bash

printf "Building libminisyncpp in a Docker container...\n"

DOCKER_IMG="minisync_build:v2.5.5"

printf "Checking if build image exists in registry: "
if [[ "$(docker images -q ${DOCKER_IMG} 2> /dev/null)" == "" ]]; then
    # check if image exists. If it doesn't, build it.
    printf "not found.\n"
    printf "Building required Docker image:\n"
    docker build --tag=${DOCKER_IMG} . -f-<<EOF
FROM ubuntu:18.10
LABEL maintainer="Manuel Olguín <molguin@kth.se>"

# install requirements
COPY ./sources.list /etc/apt/sources.list
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y wget git curl tar

# first get the Raspberry Pi cross-compilers
# done in this order because it speeds up future image builds
COPY ./install_raspi_xcompiler.sh /tmp/
RUN /tmp/install_raspi_xcompiler.sh

# install build deps
RUN apt-get install -y build-essential cmake gcc g++ clang \
    python3-all python3-all-dev libpython3-all-dev python3-setuptools
RUN update-alternatives --install /usr/bin/python python /usr/bin/python3.7 1
RUN update-alternatives --install /usr/bin/python python /usr/bin/python2.7 2
RUN update-alternatives --set python /usr/bin/python3.7
EOF

else
    printf "found.\n"
fi

LINUX_BUILD_DIR="build_x86_64";
RASPI_BUILD_DIR="build_ARMv7";
RASPI_ENV_PATH="/opt/raspi_gcc.env";
RASPI_GCC_EXEC="\$RASPI_GCC_PATH/bin/arm-linux-gnueabihf-gcc";
RASPI_GXX_EXEC="\$RASPI_GCC_PATH/bin/arm-linux-gnueabihf-g++";
RASPI_LD_LIB_PATH="\$RASPI_GCC_PATH/lib:\$LD_LIBRARY_PATH";
COMMON_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release \
-UCMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES \
-UCMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES \
-DLIBMINISYNCPP_BUILD_DEMO=TRUE \
-DLIBMINISYNCPP_BUILD_TESTS=TRUE \
-DLIBMINISYNCPP_ENABLE_LOGURU=TRUE \
"

# use the docker image to build both for Linux and for Raspberry Pi
printf "Building MiniSynCPP x86_64...\n";
docker run --name=build_x86_64 --rm -v ${PWD}:/mnt/build ${DOCKER_IMG} /bin/bash -c \
"\
cd /mnt/build;\
mkdir -p ${LINUX_BUILD_DIR};\
cd ${LINUX_BUILD_DIR};\
cmake ${COMMON_CMAKE_FLAGS} -DLIBMINISYNCPP_WITH_PYTHON=TRUE .. &&\
cmake --build . --target clean -- -j 4;\
cmake --build . --target all -- -j 4;\
./tests/minisyncpp;\
cd ..; chmod 0777 ${LINUX_BUILD_DIR};\
";

printf "Building MiniSynCPP ARMv7...\n";
docker run --name=build_ARMv7 --rm -v ${PWD}:/mnt/build ${DOCKER_IMG} /bin/bash -c \
"\
source ${RASPI_ENV_PATH};\
cd /mnt/build;\
mkdir -p ${RASPI_BUILD_DIR};\
cd ${RASPI_BUILD_DIR};\
LD_LIBRARY_PATH='${RASPI_LD_LIB_PATH}' cmake -DCMAKE_C_COMPILER=${RASPI_GCC_EXEC} \
-DCMAKE_CXX_COMPILER=${RASPI_GXX_EXEC} \
-DCMAKE_CXX_FLAGS='-march=armv7-a -mfloat-abi=hard -mfpu=neon-vfpv4' \
${COMMON_CMAKE_FLAGS} .. &&\
cmake --build . --target clean -- -j 4;\
LD_LIBRARY_PATH='${RASPI_LD_LIB_PATH}' cmake --build . --target all -- -j 4;\
cd ..; chmod 0777 ${RASPI_BUILD_DIR};\
";
printf "Done.\n";

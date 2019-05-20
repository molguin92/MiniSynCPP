#!/usr/bin/env bash

wget "https://sourceforge.net/projects/raspberry-pi-cross-compilers/files/latest/download" \
--verbose -O /tmp/gcc-pi.tar.gz;

XGCC_EXTRACTED="$(tar -xzvf /tmp/gcc-pi.tar.gz -C /opt)";

XGCC_ROOT="$(head -n1 <<< ${XGCC_EXTRACTED})";
XGCC_PATH="/opt/${XGCC_ROOT%/}";
echo "export RASPI_GCC_PATH=${XGCC_PATH}" > /opt/raspi_gcc.env;
rm -rf /tmp/gcc-pi.tar.gz;

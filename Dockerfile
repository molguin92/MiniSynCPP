FROM ubuntu:18.04
LABEL maintainer="Manuel Olguín <molguin@kth.se>"

# install requirements
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y wget build-essential cmake gcc g++ clang

# get the Raspberry Pi cross-compilers
RUN wget 'https://sourceforge.net/projects/raspberry-pi-cross-compilers/files/Raspberry%20Pi%20GCC%20Cross-Compilers/\
GCC%208.3.0/Raspberry%20Pi%203A%2B%2C%203B%2B/cross-gcc-8.3.0-pi_3%2B.tar.gz/download' -O /tmp/gcc_raspi.tar.gz
RUN tar -xzvf /tmp/gcc_raspi.tar.gz -C /opt

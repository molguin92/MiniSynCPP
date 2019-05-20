FROM ubuntu:18.10
LABEL maintainer="Manuel Olguín <molguin@kth.se>"

# install requirements
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y wget build-essential cmake gcc g++ clang git curl

# get the Raspberry Pi cross-compilers
COPY ./install_raspi_xcompiler.sh /tmp/
RUN /tmp/install_raspi_xcompiler.sh

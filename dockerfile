from ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y python3 python3-dev python3-pip cmake  git
RUN pip3 install numpy

COPY . /cpp_embedded_python

RUN cd /cpp_embedded_python && \
    git submodule update --init --recursive && \
    cmake -S . -B build && \
    cmake --build build --parallel 12

WORKDIR /cpp_embedded_python/build/bin


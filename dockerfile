from ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y python3 python3-dev python3-pip cmake  git
RUN pip3 install numpy

COPY . /cpp_embedded_python

# build dir needs to be destroyed as if the workspace had this dir
# cmake would detect different path and fail.
RUN cd /cpp_embedded_python && \
    git submodule update --init --recursive && \
    rm -rf build && \
    cmake -S . -B build && \
    cmake --build build --parallel 12

WORKDIR /cpp_embedded_python/build/bin


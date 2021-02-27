# Examples how to embed Python to CPP + multithreading

This repo contains few examples of python embedding to C++.
Most of the examples are from pybind11 documentation: 
https://pybind11.readthedocs.io/en/stable/advanced/embedding.html

Later examples demonstrate problems with multithreading and simple
simple/lazy embedding. Later examples show how proper safe multi
threading can be achieved.

Note: I don't think there is any way to get multiple concurrent
threads running Python code inside same process. GIL is created
per process, not per subinterpreter. If you needs to
use Python to process stuff generated from C++, look into
either launching separate Python processes or use of networking
such as ZeroMQ.

## Ubuntu

```
sudo apt install python3-dev python3-pip cmake
pip3 install --user numpy

git clone https://github.com/buq2/cpp_embedded_python.git
cd cpp_embedded_python
git submodule update --init --recursive

cmake -S . -B build
cmake --build build

# Examples will be compiled to bin directory
# Run for example
build\bin\multithreaded.exe
```

## Windows

Install new version of Python from https://www.python.org/downloads/
Python3 is required for these examples.

Install new CMake from: https://cmake.org/download/ 

When creating the CMake build, use same architecture as your installed Python version.
For example use following if you have 64-bit Python installed:

```
pip3 install --user numpy

git clone https://github.com/buq2/cpp_embedded_python.git
cd cpp_embedded_python
git submodule update --init --recursive

cmake -S . -B build -DCMAKE_GENERATOR_PLATFORM=x64
cmake --build build

# Examples will be compiled to bin directory
# Run for example
build\bin\multithreaded.exe
```

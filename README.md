# Examples how to embed Python to CPP + multithreading

This repo contains few examples of python embedding to C++.
Most of the early examples are from pybind11 documentation: 
https://pybind11.readthedocs.io/en/stable/advanced/embedding.html

Later examples demonstrate problems with multithreading and
simple/lazy embedding. Later examples show how proper safe multi
threading can be achieved.

Note: I don't think there is any way to get multiple concurrent
threads running Python code inside the same process. GIL is created
per process, not per subinterpreter. If you needs to
use Python to process stuff generated from C++, look into
either launching separate Python processes or use of networking
such as ZeroMQ.

As creating this repo is on going learning experience, expect bugs.
If you find some, please let me know so everyone can benefit from
your findings :)

## Ubuntu

```
sudo apt install python3-dev python3-pip cmake
pip3 install --user numpy

git clone https://github.com/buq2/cpp_embedded_python.git
cd cpp_embedded_python
git submodule update --init --recursive

cmake -S . -B build
cmake --build build --parallel 12

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
cmake --build build --parallel 12

# Examples will be compiled to bin directory
# Run for example
build\bin\multithreaded
```

# Troubleshooting

Most likely you are going to have problems related to CMake not finding Python paths or numpy.
To help with Python paths, you can set them manually. So try running
```
cmake -S . -B build -DPython_ROOT_DIR=<dir>
```

## CMake not finding correct virtualenv

Try uncommenting line `SET(Python3_FIND_VIRTUALENV ONLY)` in `CMakeLists.txt`.

## ModuleNotFoundError: No module named 'encodings'

If you get error similar to:
```
Fatal Python error: initfsencoding: unable to load the file system codec 
ModuleNotFoundError: No module named 'encodings'
```
there is something wrong with your PATH/PYTHONHOME environment variable.

Try setting env `PYTHONHOME` to directory which contains Python installation. 

For example in Windows Power Shell I had to run
```
$env:PYTHONHOME = 'C:\Users\buq2\.conda\envs\cpp_embedded_python'; 
.\build\bin\multithreaded.exe
```

## Conda

CMake has better support for conda than for example pipenv and finding all Python development files tends to work better with conda.
For more information check here: https://cmake.org/cmake/help/git-master/module/FindPython3.html#hints and take a look at `Python3_FIND_VIRTUALENV`.

Install conda from https://docs.conda.io/en/latest/miniconda.html and create a new env with.

```
conda create --name cpp_embedded_python python=3.9
conda activate cpp_embedded_python
```

Rest of the installation commands should work normally.


## Docker

Sure way to run the examples is to run them in a docker container:

```
docker build . -t cpp_embedded_python
docker run --rm cpp_embedded_python ./multithreaded
```

Build process copies the local data, so you can modify the repo and rerun the build and run commands to get different output.
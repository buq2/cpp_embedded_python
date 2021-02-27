cmake_minimum_required(VERSION 3.4)
project(example)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
IF(MSVC)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
  SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)
  SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
ENDIF(MSVC)

# Finding python separately from pybind11 produced better results for me.
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
# Most likely linking to Threads is not needed, but in some environments
# I had problems with missing pthreads stuff.
find_package (Threads REQUIRED)
add_subdirectory(pybind11)

add_executable(hello ex1_hello.cpp)
target_link_libraries(hello PRIVATE pybind11::embed)

add_executable(execute_str ex2_execute_str.cpp)
target_link_libraries(execute_str PRIVATE pybind11::embed)

add_executable(execute_str2 ex3_execute_str2.cpp)
target_link_libraries(execute_str2 PRIVATE pybind11::embed)

add_executable(import_module ex4_import_module.cpp)
target_link_libraries(import_module PRIVATE pybind11::embed)

add_executable(import_module_use_stl ex5_import_module_use_stl.cpp)
target_link_libraries(import_module_use_stl PRIVATE pybind11::embed)

add_executable(multithreaded_broken ex6_multithreaded_broken.cpp)
target_link_libraries(multithreaded_broken PRIVATE pybind11::embed Threads::Threads)

add_executable(multithreaded ex7_multithreaded.cpp)
target_link_libraries(multithreaded PRIVATE pybind11::embed Threads::Threads)
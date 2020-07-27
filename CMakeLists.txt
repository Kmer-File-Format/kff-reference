cmake_minimum_required(VERSION 3.10)

# set the project name
project(KFF)
project(KFF VERSION 1.0)
configure_file(kff_io.hpp.in kff_io.hpp)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(kff_example main.cpp kff_io.cpp)

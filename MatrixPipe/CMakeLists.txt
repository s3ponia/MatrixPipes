cmake_minimum_required(VERSION 3.17)
project(MatrixPipe)

set(CMAKE_CXX_STANDARD 17)

add_executable(MatrixPipe main.cpp Matrix.h FunctionCompose.h FlatMap.h Pipe.h)
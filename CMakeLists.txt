cmake_minimum_required(VERSION 3.17)
project(xor_crypto)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lzip")

add_executable(xor_crypto main.cpp)
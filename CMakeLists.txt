cmake_minimum_required(VERSION 3.17)
project(xor_crypto)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -lzip -ltbb")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -lzip")

add_executable(xor_crypto main.cpp)
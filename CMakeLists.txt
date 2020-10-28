cmake_minimum_required(VERSION 3.0)

project(TestConcurrency)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread" )
add_executable(TestConcurrency main.cpp)
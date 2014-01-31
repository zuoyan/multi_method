CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(multi_method)

SET(CMAKE_INCLUDE_CURRENT_DIR 1)

SET(CMAKE_CXX_FLAGS "-Wall -std=c++0x")
SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -std=c++0x -O0 -g -fno-inline")
SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -std=c++0x -O3 -march=native")

ADD_EXECUTABLE(multi_method_test multi_method_test.cc)

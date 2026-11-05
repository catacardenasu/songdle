cmake_minimum_required(VERSION 3.16)
project(Songdle CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(include src)
add_executable(songdle src/main.cpp)
add_executable(test_redblack tests/test_redblack.cpp src/RedBlackTree.cpp)

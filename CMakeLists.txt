cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_COMPILER g++)
project(cpp-pathfinder VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(cpp-pathfinder main.cpp)

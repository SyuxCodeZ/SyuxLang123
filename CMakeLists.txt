cmake_minimum_required(VERSION 3.16)
project(SyuxV13)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

add_executable(syux
  src/main.cpp
  src/parser.cpp
  src/scanner.cpp
  src/codegen.cpp
)
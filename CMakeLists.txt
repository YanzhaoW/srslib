cmake_minimum_required(VERSION 3.26)

project(
  srslib
  VERSION 0.0.1
  LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(srs)
add_subdirectory(apps)

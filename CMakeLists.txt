cmake_minimum_required(VERSION 3.26)

project(
    srs
    VERSION 0.0.1
    LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(srs)

add_library(srs INTERFACE)
target_link_libraries(srs INTERFACE srs_cpp srs_c)
target_include_directories(srs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(apps)

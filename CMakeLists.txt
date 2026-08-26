cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(nodeplot VERSION 1.0 LANGUAGES CXX)

include(FetchContent)

add_subdirectory(nodeplot)
add_subdirectory(nodeplot-gui)
add_subdirectory(nodeplot-svg)
add_subdirectory(nodeplot-pdf)
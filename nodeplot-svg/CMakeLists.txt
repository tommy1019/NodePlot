cmake_minimum_required(VERSION 3.25)

set(SOURCES
    src/main.cpp
)

add_executable(nodeplot-svg ${SOURCES})

target_link_libraries(nodeplot-svg PUBLIC nodeplot)

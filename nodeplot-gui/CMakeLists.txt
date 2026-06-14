cmake_minimum_required(VERSION 3.25)

set(SOURCES
    src/main.cpp
    src/window.cpp
    src/svg_renderer.cpp
    src/node_renderer.cpp
)

add_executable(nodeplot-gui ${SOURCES})

target_link_libraries(nodeplot-gui PUBLIC nodeplot lunasvg::lunasvg imgui nfd)

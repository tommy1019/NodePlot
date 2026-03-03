cmake_minimum_required(VERSION 3.25)

set(SOURCES
    src/main.cpp
    src/window.cpp
    src/render_node_graph.cpp
    src/svg_renderer.cpp

    src/node_impl/output.cpp
)

add_executable(nodeplot-gui ${SOURCES})

target_link_libraries(nodeplot-gui PUBLIC nodeplot lunasvg::lunasvg imgui portable_file_dialogs)

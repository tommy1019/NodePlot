cmake_minimum_required(VERSION 3.25)

set(SOURCES
    src/main.cpp
    src/window.cpp
    src/render_node_graph.cpp
    src/svg_renderer.cpp

    src/node_impl/csv_import.cpp
    src/node_impl/filter_table.cpp
    src/node_impl/column_select.cpp
    src/node_impl/sampled_property_extract.cpp
    src/node_impl/series_create.cpp
    src/node_impl/create_graph.cpp
    src/node_impl/output.cpp
)

add_executable(nodeplot-gui ${SOURCES})

target_link_libraries(nodeplot-gui PUBLIC nodeplot lunasvg::lunasvg imgui portable_file_dialogs)

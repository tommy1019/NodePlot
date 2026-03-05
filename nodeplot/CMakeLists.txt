cmake_minimum_required(VERSION 3.25)

set(SOURCES
    src/nodeplot.cpp

    src/node_impl/csv_import.cpp
    src/node_impl/filter_table.cpp
    src/node_impl/column_select.cpp
    src/node_impl/binary_operation.cpp
    src/node_impl/sampled_property_extract.cpp
    src/node_impl/series_create.cpp
    src/node_impl/create_graph.cpp
    src/node_impl/output.cpp
)

add_library(nodeplot ${SOURCES})
target_link_libraries(nodeplot PUBLIC nlohmann_json::nlohmann_json)
target_include_directories(nodeplot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/nodeplot)
target_include_directories(nodeplot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

#include "../render_node.h"

template <>
void node_render(RenderNodeGraph* rng, ColumnSelectNode& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            input("##i_column_name", "##ii_column_name", "Column Name", node.i_column_name);
            input("##i_table", "##ii_table", "Table", node.i_table);
        },
        [&](auto output) { output("##o_column", 0, "Column"); });
}

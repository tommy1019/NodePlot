#include "../render_node.h"

template <>
void node_render(RenderNodeGraph* rng, CSVImportNode& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            input("##i_source_path", "##ii_source_path", "Source Path", node.i_source_path);
            input("##i_has_headers", "##ii_has_headers", "Has Headers", node.i_has_headers);
        },
        [&](auto output) { output("##o_table_data", 0, "Table Data"); });
}

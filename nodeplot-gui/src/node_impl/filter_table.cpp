#include "../render_node.h"
#include "nodeplot/nodeplot.h"

template <>
void node_render(RenderNodeGraph* rng, FilterTableNode& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input, auto input_custom) {
            input("##i_column_name", "##ii_column_name", "Column Name", node.i_column_name);
            input("##i_table", "##ii_table", "Table", node.i_table);
            input("##i_numeric_compare", "##ii_numeric_compare", "Numeric Comparision", node.i_numeric_compare);
            input("##i_compare_value", "##ii_compare_value", "Compare Value", node.i_compare_value);
            input_custom("##i_compare_type", "##ii_compare_type", "Compare Type", node.i_compare_type, [&](const char* input_id, int32_t& input) {
                static const char* items[] = {"Less Than", "Less Than or Equal To", "Equal", "Not Equal", "Greater Than or Equal To", "Gerater Than"};
                return ImGui::Combo(input_id, &input, items, IM_ARRAYSIZE(items));
            });
        },
        [&](auto output) { output("##o_table", 0, "Table"); });
}

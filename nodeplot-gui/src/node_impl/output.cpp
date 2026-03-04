#include "../render_node.h"
#include "imgui.h"

template <>
void node_render(RenderNodeGraph* rng, OutputNode& node) {
    // TODO: This will crash if grid cols or grid rows comes from pin input

    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input, auto input_custom) {
            int32_t rows = std::get<int32_t>(node.i_grid_rows);
            int32_t cols = std::get<int32_t>(node.i_grid_cols);

            for (size_t i = 0; i < node.i_graphs.size(); i++) {
                ImGui::PushID(i);
                input("##i_graph", "##ii_graph", std::format("Graph ({}, {})", (i % cols) + 1, (i / cols) + 1).c_str(), node.i_graphs[i]);
                ImGui::PopID();
            }

            input("##i_output_filename", "##ii_output_filename", "Output Filename", node.i_output_filename);
            input("##i_width", "##ii_width", "Width", node.i_width);
            input("##i_height", "##ii_height", "Height", node.i_height);
            input_custom("##i_grid_cols", "##ii_grid_cols", "Grid Cols", node.i_grid_cols, [&](const char* input_id, int32_t& input) {
                ImGui::SetNextItemWidth(200 * rng->scene_scale);
                if (ImGui::InputInt(input_id, &input)) {
                    node.i_graphs.resize(std::get<int32_t>(node.i_grid_cols) * std::get<int32_t>(node.i_grid_rows));
                    return true;
                }
                return false;
            });
            input_custom("##i_grid_rows", "##ii_grid_rows", "Grid Rows", node.i_grid_rows, [&](const char* input_id, int32_t& input) {
                ImGui::SetNextItemWidth(200 * rng->scene_scale);
                if (ImGui::InputInt(input_id, &input)) {
                    node.i_graphs.resize(std::get<int32_t>(node.i_grid_cols) * std::get<int32_t>(node.i_grid_rows));
                    return true;
                }
                return false;
            });
        },
        [&](auto output) {});

    if (ImGui::BeginPopupContextItem("output_context")) {
        if (ImGui::MenuItem("Visualize Output")) {
            rng->eval_node_graph.node_graph.visualize_node() = node.id;
            rng->update_target(node.id);
        }
        ImGui::EndPopup();
    }

    if (rng->eval_node_graph.node_graph.visualize_node() == node.id) {
        ImGui::Text("Visualizing...");
    }
}
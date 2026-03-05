#include "../render_node.h"
#include "imgui.h"
#include "nodeplot/nodeplot.h"

template <>
void node_render(RenderNodeGraph* rng, OutputNode& node) {
    // TODO: This will crash if grid cols or grid rows comes from pin input

    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            int32_t rows = std::get<int32_t>(node.i_grid_rows);
            int32_t cols = std::get<int32_t>(node.i_grid_cols);

            for (size_t i = 0; i < node.i_graphs.size(); i++) {
                ImGui::PushID(i);
                input(node.i_graphs[i], "graph", std::format("Graph ({}, {})", (i % cols) + 1, (i / cols) + 1).c_str(), default_input_element);
                ImGui::PopID();
            }

            input(node.i_output_filename, "output_filename", "Output Filename", default_input_element);
            input(node.i_width, "width", "Width", default_input_element);
            input(node.i_height, "height", "Height", default_input_element);
            input(node.i_grid_cols, "grid_cols", "Grid Cols", [&](RenderNodeGraph* rng, OutputNode& node, const char* input_id, int32_t& input) {
                ImGui::SetNextItemWidth(200 * rng->scene_scale);
                if (ImGui::InputInt("##", &input)) {
                    node.i_graphs.resize(std::get<int32_t>(node.i_grid_cols) * std::get<int32_t>(node.i_grid_rows));
                    return true;
                }
                return false;
            });
            input(node.i_grid_rows, "grid_rows", "Grid Rows", [&](RenderNodeGraph* rng, OutputNode& node, const char* input_id, int32_t& input) {
                ImGui::SetNextItemWidth(200 * rng->scene_scale);
                if (ImGui::InputInt("##", &input)) {
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
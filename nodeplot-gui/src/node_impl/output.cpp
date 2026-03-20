#include "../render_node.h"
#include "imgui.h"
#include "nodeplot/nodeplot.h"

template <>
void node_render(RenderNodeGraph* rng, OutputNode& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            void* empty = nullptr;
            input(empty, "dummy", "Graphs", [&](RenderNodeGraph* rng, OutputNode& node, const char* input_id, auto& storage) { return false; });

            for (size_t i = 0; i < node.i_graphs.size(); i++) {
                ImGui::PushID(i);
                input(node.i_graphs[i], "graph", std::format("Graph ({}, {})", (i % node.i_grid_cols) + 1, (i / node.i_grid_cols) + 1).c_str(), default_input_element);
                ImGui::PopID();
            }

            input(node.i_output_filename, "output_filename", "Output Filename", default_input_element);
            input(node.i_width, "width", "Width", default_input_element);
            input(node.i_height, "height", "Height", default_input_element);
            input(node.i_grid_cols, "grid_cols", "Grid Cols", [&](RenderNodeGraph* rng, OutputNode& node, const char* input_id, int32_t& input) {
                if (ImGui::InputInt("##", &input)) {
                    node.i_graphs.resize(node.i_grid_cols * node.i_grid_rows);
                    return true;
                }
                return false;
            });
            input(node.i_grid_rows, "grid_rows", "Grid Rows", [&](RenderNodeGraph* rng, OutputNode& node, const char* input_id, int32_t& input) {
                if (ImGui::InputInt("##", &input)) {
                    node.i_graphs.resize(node.i_grid_cols * node.i_grid_rows);
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
#pragma once

#include "nodeplot/types.h"
#include <functional>
#include <nodeplot/nodeplot.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <vector>

struct NodeRenderer {

    struct NodeCache {
        bool style_open = false;
    };

    struct RenderContext {
        NodeRenderer& node_renderer;
        NodePlot::NodePlotFile* npf;
        NodePlot::EvaluatedNodeGraph* eng;

        NodePlot::NodeId node_id;
        NodePlot::NodeGraph::NodeStorage& node_storage;

        NodePlot::Node node;

        NodeCache& cache;
    };

    struct Renderer {
        std::function<void(std::string)> push_id;
        std::function<void(std::string)> pop_id;

        std::function<void(RenderContext&, ImVec2, std::string)> text = [](RenderContext&, ImVec2, std::string) {};
        std::function<void(RenderContext&, ImVec2)> separator = [](RenderContext&, ImVec2) {};

        std::function<std::optional<bool>(RenderContext&, std::string, std::function<void()>)> folded_section = [](RenderContext& ctx, std::string, std::function<void()> f) {
            f();
            return std::nullopt;
        };

        std::function<bool(RenderContext&, ImVec2, ImVec2, std::string)> button = [](RenderContext&, ImVec2, ImVec2, std::string) { return false; };
        std::function<std::optional<std::string>(RenderContext&, ImVec2, ImVec2, std::string, std::vector<std::pair<std::string, std::string>>)> dropdown
            = [](RenderContext&, ImVec2, ImVec2, std::string, std::vector<std::pair<std::string, std::string>>) { return std::nullopt; };

        std::function<bool(RenderContext&, ImVec2, float, NodePlot::InputId, std::optional<NodePlot::Data>)> input_pin
            = [](RenderContext&, ImVec2, float, NodePlot::InputId, std::optional<NodePlot::Data>) { return false; };
        std::function<bool(RenderContext&, ImVec2, float, NodePlot::OutputId)> output_pin = [](RenderContext&, ImVec2, float, NodePlot::OutputId) { return false; };

        std::function<bool(RenderContext&, ImVec2, ImVec2, std::string&, bool)> string_input = [](RenderContext&, ImVec2, ImVec2, std::string&, bool) { return false; };
        std::function<bool(RenderContext&, ImVec2, bool&, bool)> boolean_input = [](RenderContext&, ImVec2, bool&, bool) { return false; };
        std::function<bool(RenderContext&, ImVec2, ImVec2, double&, bool)> number_input = [](RenderContext&, ImVec2, ImVec2, double&, bool) { return false; };
        std::function<bool(RenderContext&, ImVec2, ImVec2, int64_t&, bool)> integer_input = [](RenderContext&, ImVec2, ImVec2, int64_t&, bool) { return false; };
        std::function<bool(RenderContext&, ImVec2, ImVec2, NodePlot::Color&, bool)> color_input = [](RenderContext&, ImVec2, ImVec2, NodePlot::Color&, bool) { return false; };
        std::function<bool(RenderContext&, ImVec2, ImVec2, NodePlot::Margins&, bool)> margin_input = [](RenderContext&, ImVec2, ImVec2, NodePlot::Margins&, bool) { return false; };
        std::function<bool(RenderContext&, ImVec2, ImVec2, NodePlot::Pos&, bool)> position_input = [](RenderContext&, ImVec2, ImVec2, NodePlot::Pos&, bool) { return false; };
    };

    using RenderFunction = std::function<ErrorOr<bool>(Renderer&, RenderContext&)>;

    static std::map<NodePlot::NodeTypeId, RenderFunction> render_override_map;
    static RenderFunction default_renderer;

    std::map<NodePlot::NodeId, NodeCache> node_cache;

    float scene_scale = 1;
    ImVec2 scene_translation = {};

    ImVec2 world_to_screen(ImVec2 p) { return ImVec2((p.x - scene_translation.x) * scene_scale, (p.y - scene_translation.y) * scene_scale); };
    ImVec2 screen_to_world(ImVec2 p) { return ImVec2(p.x / scene_scale + scene_translation.x, p.y / scene_scale + scene_translation.y); };

    NodePlot::NodePlotFile* npf;
    NodePlot::EvaluatedNodeGraph* eng;

    // TODO: this is a bad way to do a callback, refractor this
    std::function<void(NodePlot::GraphId)> set_current_graph_id = [](NodePlot::GraphId) {};

    NodeRenderer(NodePlot::NodePlotFile* npf, NodePlot::EvaluatedNodeGraph* eng) : npf(npf), eng(eng) {}

    void draw_node_path(float start_node_pos, ImVec2 start, ImVec2 end, float end_node_pos);

    ErrorOr<bool> render_node(NodePlot::NodeId node_id, NodePlot::NodeGraph::NodeStorage& storage);

    ErrorOr<void> render_input_paths(NodePlot::NodeId node_id, NodePlot::NodeGraph::NodeStorage& storage);
};

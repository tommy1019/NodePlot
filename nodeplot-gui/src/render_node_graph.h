#pragma once

#include <nodeplot/nodeplot.h>

#include "svg_renderer.h"

struct RenderNodeGraph {
    float scene_scale = 1;
    ImVec2 scene_translation = {};

    EvaluatedNodeGraph eval_node_graph;

    std::shared_ptr<SVGRenderer> svg_renderer;

    struct NodeRenderInfo {
        std::map<OutputId, ImVec2> render_pin_positions;
    };
    std::map<int64_t, NodeRenderInfo> render_nodes;

    void update_target(NodeId updated_node = -1);

    ImVec2 world_to_screen(ImVec2 p) { return ImVec2((p.x - scene_translation.x) * scene_scale, (p.y - scene_translation.y) * scene_scale); }
    ImVec2 screen_to_world(ImVec2 p) { return ImVec2(p.x / scene_scale + scene_translation.x, p.y / scene_scale + scene_translation.y); }
};

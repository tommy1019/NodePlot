#pragma once

#include <nodeplot/nodeplot.h>

#include "svg_renderer.h"

struct RenderNodeGraph {
    ImVec2 scene_translation = {};

    EvaluatedNodeGraph eval_node_graph;

    std::shared_ptr<SVGRenderer> svg_renderer;

    struct NodeRenderInfo {
        std::map<OutputId, ImVec2> render_pin_positions;
    };
    std::map<int64_t, NodeRenderInfo> render_nodes;

    void update_target(NodeId updated_node = -1);
};

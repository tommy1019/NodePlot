#pragma once

#include "node.h"
#include "types.h"

namespace NodePlot {
struct NodeRegistry {
    static std::map<NodeTypeId, Node> node_map;

    static void init();

    static void register_node(NodeTypeId node_id, Node node);
};

} // namespace NodePlot
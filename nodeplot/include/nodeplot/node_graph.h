#pragma once

#include "types.h"

#include <nlohmann/json.hpp>

namespace NodePlot {

struct NodeGraph {
    struct InputPin {
        NodeId node_id;
        OutputId output_id;
    };

    struct NodeStorage {
        NodeTypeId type_id;

        struct {
            float x = 0;
            float y = 0;
        } pos;

        std::map<InputId, std::variant<Data, InputPin>> input_storage;
    };

    std::map<NodeId, NodeStorage> nodes;
    NodeId next_free_node_id = 0;

    static ErrorOr<NodeGraph> from_json(nlohmann::json json);

    ErrorOr<nlohmann::json> to_json();

    ErrorOr<NodeId> create_node(NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeTypeId type, float x, float y);
};

} // namespace NodePlot
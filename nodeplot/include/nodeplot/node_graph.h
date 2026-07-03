#pragma once

#include <optional>

#include <nlohmann/json.hpp>

#include "error.h"
#include "types.h"

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

        struct {
            float x = 0;
            float y = 0;
        } end_pos;

        std::map<InputId, std::variant<Data, InputPin>> input_storage;
    };

    std::map<NodeId, NodeStorage> nodes;
    NodeId next_free_node_id = 0;

    std::optional<NodeId> function_input_id = std::nullopt;
    std::optional<NodeId> function_output_id = std::nullopt;

    static ErrorOr<NodeGraph> from_json(nlohmann::json json);

    ErrorOr<nlohmann::json> to_json();

    ErrorOr<NodeId> create_node(NodePlotFile* npf, GraphId graph_id, NodeTypeId type, float x, float y);
};

} // namespace NodePlot

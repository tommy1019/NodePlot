#include "nodeplot.h"

template <>
ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const IntegerValueNode& node) {
    auto res = TRY_OR(graph->get_input(id.graph_name, node.i_val), return ERR("Could not get 'Value' input"));
    return std::map<OutputIndex, NodeOutput>{{"value", res}};
}

template <>
ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const NumericValueNode& node) {
    auto res = TRY_OR(graph->get_input(id.graph_name, node.i_val), return ERR("Could not get 'Value' input"));
    return std::map<OutputIndex, NodeOutput>{{"value", res}};
}

template <>
ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const StringValueNode& node) {
    auto res = TRY_OR(graph->get_input(id.graph_name, node.i_val), return ERR("Could not get 'Value' input"));
    return std::map<OutputIndex, NodeOutput>{{"value", res}};
}

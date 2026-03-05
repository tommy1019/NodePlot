#include "nodeplot.h"

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const IntegerValueNode& node, OutputIndex id) {
    return TRY_OR(graph->get_input(node.i_val), return ERR("Could not get 'Value' input"));
}

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const NumericValueNode& node, OutputIndex id) {
    return TRY_OR(graph->get_input(node.i_val), return ERR("Could not get 'Value' input"));
}

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const StringValueNode& node, OutputIndex id) {
    return TRY_OR(graph->get_input(node.i_val), return ERR("Could not get 'Value' input"));
}

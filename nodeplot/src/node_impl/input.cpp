#include "nodeplot.h"
#include "nodes.h"

template <>
ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const InputNode& node) {
    return ERR("Trying to get output from an input node");
}

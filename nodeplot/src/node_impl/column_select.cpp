#include "error.h"
#include "nodeplot.h"

template <>
ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const ColumnSelectNode& node) {
    auto table = TRY_OR(graph->get_input(id.graph_name, node.i_table), return ERR("Could not get 'Table' input"));
    auto column_name = TRY_OR(graph->get_input(id.graph_name, node.i_column_name), return ERR("Could not get 'Column Name' input"));

    auto res = table.columns.find(column_name.name);
    if (res == table.columns.end())
        return ERR("No column by the specified name found");

    return std::map<OutputIndex, NodeOutput>{{"column", res->second}};
}

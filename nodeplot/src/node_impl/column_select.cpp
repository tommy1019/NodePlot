#include "error.h"
#include "nodeplot.h"

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const ColumnSelectNode& node, OutputId id) {
    auto table = TRY_OR(graph->get_input(node.i_table), return ERR("Could not get 'Table' input"));
    auto column_name = TRY_OR(graph->get_input(node.i_column_name), return ERR("Could not get 'Column Name' input"));

    auto res = table.columns.find(column_name.name);
    if (res == table.columns.end())
        return ERR("No column by the specified name found");

    return res->second;
}

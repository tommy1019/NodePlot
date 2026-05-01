#include "error.h"
#include "nodeplot.h"

using namespace NodePlot;

void register_column_select() {
    NodeRegistry::register_node(
        "column_select",
        Node{
            .type_id = "column_select",
            .display_name = "Column Select",
            .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                return {
                    {"table", Node::Input{.id = "table", .display_name = "Table", .valid_data_types = {DataType::TABLE}}},
                    {"column_name",
                     Node::Input{.id = "column_name", .display_name = "Column Name", .valid_data_types = {DataType::STRING}, .attributes = {InputAttribute::AutoFillsFromTable{.id = "table"}}}},
                };
            },
            .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                return {
                    {"column", Node::Output{.id = "column", .display_name = "Column", .valid_data_types = {DataType::COLUMN}}},
                };
            },
            .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                Table table = TRY(eng->get_input_value<Table>(npf, node_id, "table"));
                std::string column_name = TRY(eng->get_input_value<std::string>(npf, node_id, "column_name"));

                auto res = table.columns.find(column_name);
                if (res == table.columns.end())
                    return ERR("No column by the specified name found");

                cache.computed_outputs["column"] = res->second;

                return {};
            },
        });
}

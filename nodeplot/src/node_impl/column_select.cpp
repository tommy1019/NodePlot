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
                    {"column", Node::Output{.id = "column", .display_name = "Column", .valid_data_types = {DataType::STRING_COLUMN}}},
                };
            },
            .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                Table table = TRY(eng->get_input_value<Table>(npf, node_id, "table", true));
                std::string column_name = TRY(eng->get_input_value<std::string>(npf, node_id, "column_name", true));

                auto res = table->columns.find(column_name);
                if (res == table->columns.end()) {
                    return std::unexpected(Node::EvaluationError{
                        .description = "Table does not contain a column called '" + column_name + "'",
                        .input_errors = {"column_name"},
                    });
                }

                output["column"] = res->second;

                return {};
            },
        });
}

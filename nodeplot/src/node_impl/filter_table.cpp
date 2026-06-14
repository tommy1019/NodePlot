#include "error.h"
#include "nodeplot.h"
#include "utils.h"
#include <string>
#include <variant>
#include <vector>

using namespace NodePlot;

void register_filter_table() {
    NodeRegistry::register_node(
        "filter_table",
        Node{
            .type_id = "filter_table",
            .display_name = "Filter Table",
            .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                std::vector<std::pair<InputId, Node::Input>> res;

                res.emplace_back("table", Node::Input{.id = "table", .display_name = "Table", .valid_data_types = {DataType::TABLE}});
                res.emplace_back(
                    "column_name",
                    Node::Input{.id = "column_name", .display_name = "Column Name", .valid_data_types = {DataType::STRING}, .attributes = {InputAttribute::AutoFillsFromTable{.id = "table"}}});
                res.emplace_back("compare_type", Node::Input{.id = "compare_type", .display_name = "Compare Type", .valid_data_types = {DataType::STRING}, .default_value = "<"});
                res.emplace_back("numeric_compare", Node::Input{.id = "numeric_compare", .display_name = "Numeric Compare", .valid_data_types = {DataType::BOOLEAN}, .default_value = true});

                if (eng->get_input_value<bool>(npf, node_id, "numeric_compare").value_or(false)) {
                    res.emplace_back("compare_value", Node::Input{.id = "compare_value", .display_name = "Compare Value", .valid_data_types = {DataType::NUMBER}});
                } else {
                    res.emplace_back("compare_value", Node::Input{.id = "compare_value", .display_name = "Compare Value", .valid_data_types = {DataType::STRING}});
                }

                return res;
            },
            .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                return {
                    {"table", Node::Output{.id = "table", .display_name = "Table", .valid_data_types = {DataType::TABLE}}},
                };
            },
            .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                Table table = TRY(eng->get_input_value<Table>(npf, node_id, "table"));
                std::string column_name = TRY(eng->get_input_value<std::string>(npf, node_id, "column_name"));
                std::string compare_type = TRY(eng->get_input_value<std::string>(npf, node_id, "compare_type"));
                bool numeric_compare = TRY(eng->get_input_value<bool>(npf, node_id, "numeric_compare"));

                auto& column = TRY(Utils::try_find(table->columns, column_name, "No column by the specified name found")).get();

                std::vector<bool> compared_column;
                compared_column.reserve(column.size());

                if (numeric_compare) {
                    double compare_val = TRY(eng->get_input_value<double>(npf, node_id, "compare_value"));
                    for (auto v : column) {
                        double num_v = Utils::string_to_double_with_nan(v);
                        if (compare_type == "<") {
                            compared_column.push_back(num_v < compare_val);
                        } else if (compare_type == "<=") {
                            compared_column.push_back(num_v <= compare_val);
                        } else if (compare_type == "==") {
                            compared_column.push_back(num_v == compare_val);
                        } else if (compare_type == "!=") {
                            compared_column.push_back(num_v != compare_val);
                        } else if (compare_type == ">=") {
                            compared_column.push_back(num_v >= compare_val);
                        } else if (compare_type == ">") {
                            compared_column.push_back(num_v > compare_val);
                        } else {
                            return ERR("Invalid comparision type");
                        }
                    }
                } else {
                    std::string compare_val = TRY(eng->get_input_value<std::string>(npf, node_id, "compare_value"));
                    for (auto v : column) {
                        if (compare_type == "<") {
                            compared_column.push_back(v < compare_val);
                        } else if (compare_type == "<=") {
                            compared_column.push_back(v <= compare_val);
                        } else if (compare_type == "==") {
                            compared_column.push_back(v == compare_val);
                        } else if (compare_type == "!=") {
                            compared_column.push_back(v != compare_val);
                        } else if (compare_type == ">=") {
                            compared_column.push_back(v >= compare_val);
                        } else if (compare_type == ">") {
                            compared_column.push_back(v > compare_val);
                        } else {
                            return ERR("Invalid comparision type");
                        }
                    }
                }

                size_t retain_amt = 0;
                for (auto v : compared_column)
                    if (v)
                        retain_amt++;

                Table res(new TableData);
                res->column_names = table->column_names;

                for (auto& c : table->columns) {
                    res->columns[c.first] = [&](std::vector<std::string> col) {
                        std::vector<std::string> res;
                        res.reserve(retain_amt);

                        for (size_t i = 0; i < compared_column.size(); i++)
                            if (compared_column[i])
                                res.push_back(col[i]);

                        return res;
                    }(c.second);
                }

                cache.computed_outputs["table"] = res;

                return {};
            },
        });
}

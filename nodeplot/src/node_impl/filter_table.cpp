#include "error.h"
#include "nodeplot.h"
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
            .evalulate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, NodeOutputCache& cache) -> ErrorOr<void> {
                Table table = TRY(eng->get_input_value<Table>(npf, node_id, "table"));
                std::string column_name = TRY(eng->get_input_value<std::string>(npf, node_id, "column_name"));
                std::string compare_type = TRY(eng->get_input_value<std::string>(npf, node_id, "compare_type"));
                bool numeric_compare = TRY(eng->get_input_value<bool>(npf, node_id, "numeric_compare"));

                auto column = table.columns.find(column_name);
                if (column == table.columns.end())
                    return ERR("No column by the specified name found");

                auto comparision_check = [&](auto col, auto compare_val) -> ErrorOr<std::vector<bool>> {
                    std::vector<bool> res;
                    res.reserve(col.values.size());
                    for (auto& v : col.values) {
                        if (compare_type == "<") {
                            res.push_back(v < compare_val);
                        } else if (compare_type == "<=") {
                            res.push_back(v <= compare_val);
                        } else if (compare_type == "==") {
                            res.push_back(v == compare_val);
                        } else if (compare_type == "!=") {
                            res.push_back(v != compare_val);
                        } else if (compare_type == ">=") {
                            res.push_back(v >= compare_val);
                        } else if (compare_type == ">") {
                            res.push_back(v > compare_val);
                        } else {
                            return ERR("Invalid comparision type");
                        }
                    }
                    return res;
                };

                auto compare_vals = TRY([&]() -> ErrorOr<std::vector<bool>> {
                    if (numeric_compare) {
                        return comparision_check(TRY(column->second.as_numeric_column()), TRY(eng->get_input_value<double>(npf, node_id, "compare_value")));
                    } else {
                        return std::visit(
                            overloaded{
                                [&](Column::CSVImported col) -> ErrorOr<std::vector<bool>> { return comparision_check(col, TRY(eng->get_input_value<std::string>(npf, node_id, "compare_value"))); },
                                [](Column::Numeric col) -> ErrorOr<std::vector<bool>> { return ERR("Cannot compare a numeric column with a string"); },
                            },
                            column->second.raw_column);
                        return comparision_check(TRY(column->second.as_numeric_column()), TRY(eng->get_input_value<double>(npf, node_id, "compare_value")));
                    }
                }());

                size_t retain_amt = 0;
                for (auto v : compare_vals)
                    if (v)
                        retain_amt++;

                Table res;
                res.column_names = table.column_names;

                for (auto& c : table.columns) {
                    res.columns[c.first] = std::visit(
                        [&](auto raw_col) {
                            decltype(raw_col) res;
                            res.values.reserve(retain_amt);

                            for (size_t i = 0; i < compare_vals.size(); i++)
                                if (compare_vals[i])
                                    res.values.push_back(raw_col.values[i]);
                            return Column{res};
                        },
                        c.second.raw_column);
                }

                cache.computed_outputs["table"] = res;

                return {};
            },
        });
}
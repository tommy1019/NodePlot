#include "error.h"
#include "nodeplot.h"
#include <string>
#include <string_view>
#include <vector>

template <>
ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const FilterTableNode& node) {
    auto table = TRY_OR(graph->get_input(id.graph_name, node.i_table), return ERR("Could not get 'Table' input"));
    auto column_name = TRY_OR(graph->get_input(id.graph_name, node.i_column_name), return ERR("Could not get 'Column Name' input"));
    auto compare_type = TRY_OR(graph->get_input(id.graph_name, node.i_compare_type), return ERR("Could not get 'Compare Type' input"));
    auto numeric_compare = TRY_OR(graph->get_input(id.graph_name, node.i_numeric_compare), return ERR("Could not get 'Numeric Compare' input"));
    auto compare_value = TRY_OR(graph->get_input(id.graph_name, node.i_compare_value), return ERR("Could not get 'Compare Value' input"));

    auto compare_col = table.columns.find(column_name.name);
    if (compare_col == table.columns.end())
        return ERR("No column by the specified name found");

    auto& col = compare_col->second;

    double numeric_compare_val;

    if (numeric_compare) {
        col = {TRY(graph->column_as_numeric(col))};
        numeric_compare_val = TRY_OR(string_to_double(compare_value), return ERR("Compare value is not a valid number"));
    }

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

    std::vector<bool> compare_vals = TRY(std::visit(overloaded{
                                                        [&](ColumnNumeric col, double compare_val) -> ErrorOr<std::vector<bool>> { return comparision_check(col, compare_val); },
                                                        [&](ColumnCSVImported col, std::string_view compare_val) -> ErrorOr<std::vector<bool>> { return comparision_check(col, compare_val); },
                                                        [](auto, auto) -> ErrorOr<std::vector<bool>> { return ERR("Column types can't be compared"); },
                                                    },
                                                    col.raw_column,
                                                    numeric_compare ? Column::ValueType{numeric_compare_val} : Column::ValueType{compare_value}));

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

    return std::map<OutputIndex, NodeOutput>{{"table", res}};
}

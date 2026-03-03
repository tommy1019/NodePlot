#include "error.h"
#include "nodeplot.h"
#include <vector>

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const FilterTableNode& node, OutputId id) {
    auto table = TRY_OR(graph->get_input(node.i_table), return ERR("Could not get 'Table' input"));
    auto column_name = TRY_OR(graph->get_input(node.i_column_name), return ERR("Could not get 'Column Name' input"));
    auto compare_type = TRY_OR(graph->get_input(node.i_compare_type), return ERR("Could not get 'Compare Type' input"));
    auto numeric_compare = TRY_OR(graph->get_input(node.i_numeric_compare), return ERR("Could not get 'Numeric Compare' input"));
    auto compare_value = TRY_OR(graph->get_input(node.i_compare_value), return ERR("Could not get 'Compare Value' input"));

    auto compare_col = table.columns.find(column_name.name);
    if (compare_col == table.columns.end())
        return ERR("No column by the specified name found");

    auto& col = compare_col->second;

    Table res;
    res.column_names = table.column_names;
    res.mapped_file = table.mapped_file;

    double numeric_compare_val;

    if (numeric_compare) {
        col.ensure_numeric();
        numeric_compare_val = TRY_OR(string_to_double(compare_value), return ERR("Compare value is not a valid number"));
    }

    size_t amt = 0;

    auto comparision_check = [&](size_t i) -> bool {
        if (numeric_compare)
            switch (compare_type) {
            case 0:
                return (*col.numeric_values)[i] < numeric_compare_val;
            case 1:
                return (*col.numeric_values)[i] <= numeric_compare_val;
            case 2:
                return (*col.numeric_values)[i] == numeric_compare_val;
            case 3:
                return (*col.numeric_values)[i] != numeric_compare_val;
            case 4:
                return (*col.numeric_values)[i] >= numeric_compare_val;
            case 5:
                return (*col.numeric_values)[i] > numeric_compare_val;
            default:
                return false;
            }
        else
            switch (compare_type) {
            case 0:
                return col.values[i] < compare_value;
            case 1:
                return col.values[i] <= compare_value;
            case 2:
                return col.values[i] == compare_value;
            case 3:
                return col.values[i] != compare_value;
            case 4:
                return col.values[i] >= compare_value;
            case 5:
                return col.values[i] > compare_value;
            default:
                return false;
            };
    };

    std::vector<bool> compare_vals;
    compare_vals.reserve(col.values.size());

    for (size_t i = 0; i < col.values.size(); i++) {
        if (comparision_check(i)) {
            amt++;
            compare_vals.push_back(true);
        } else {
            compare_vals.push_back(false);
        }
    }

    if (amt == col.values.size())
        return table;

    for (auto& c : table.columns) {
        Column n;
        n.values.reserve(amt);

        for (size_t i = 0; i < compare_vals.size(); i++)
            if (compare_vals[i])
                n.values.push_back(c.second.values[i]);

        res.columns[c.first] = n;
    }

    return res;
}

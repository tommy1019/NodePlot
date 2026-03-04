#include "nodeplot.h"

#include <string_view>
#include <utility>
#include <vector>

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const SampledPropertyExtractNode& node, OutputIndex id) {
    auto x = TRY_OR(graph->get_input(node.i_x), return ERR("Could not get 'X' input"));
    auto y = TRY_OR(graph->get_input(node.i_y), return ERR("Could not get 'Y' input"));

    x.ensure_numeric();
    y.ensure_numeric();

    struct Bucket {
        std::vector<double> vals;

        std::string_view x_str;
        double x_val;
    };

    std::map<double, Bucket> bucketed_vals_map;

    for (size_t i = 0; i < x.values.size(); i++) {
        double v = (*x.numeric_values)[i];
        if (!bucketed_vals_map.contains(v)) {
            bucketed_vals_map.insert({v,
                                      Bucket{
                                          .x_str = x.values[i],
                                          v,
                                      }});
        }
        bucketed_vals_map[v].vals.push_back((*y.numeric_values)[i]);
    }

    std::vector<Bucket> bucketed_vals;
    for (auto& b : bucketed_vals_map)
        bucketed_vals.push_back(b.second);
    std::sort(bucketed_vals.begin(), bucketed_vals.end(), [](Bucket& l, Bucket& r) { return l.x_val < r.x_val; });

    Column col_new_x;
    Column col_min;
    Column col_avg;
    Column col_stdev;
    Column col_max;

    col_new_x.mapped_file = x.mapped_file;
    col_min.mapped_file = y.mapped_file;
    col_avg.mapped_file = y.mapped_file;
    col_stdev.mapped_file = y.mapped_file;
    col_max.mapped_file = y.mapped_file;

    col_new_x.name = x.name;
    col_min.name = y.name + " Minimum";
    col_avg.name = y.name + " Average";
    col_stdev.name = y.name + " Standard Deviation";
    col_max.name = y.name + " Maximum";

    col_new_x.numeric_values = std::vector<double>{};
    col_min.numeric_values = std::vector<double>{};
    col_avg.numeric_values = std::vector<double>{};
    col_stdev.numeric_values = std::vector<double>{};
    col_max.numeric_values = std::vector<double>{};

    for (auto& bucket : bucketed_vals) {
        double sum = 0;
        double min = bucket.vals.size() > 0 ? bucket.vals.front() : 0.0;
        double max = bucket.vals.size() > 0 ? bucket.vals.front() : 0.0;

        for (auto& v : bucket.vals) {
            sum += v;
            min = std::min(min, v);
            max = std::max(max, v);
        }

        double avg = sum / bucket.vals.size();

        sum = 0;
        for (auto& v : bucket.vals) {
            sum += std::pow(v - avg, 2);
        }
        double stdev = std::sqrt(sum / bucket.vals.size());

        col_new_x.values.push_back(bucket.x_str);
        col_new_x.numeric_values->push_back(bucket.x_val);

        col_min.values.emplace_back();
        col_min.numeric_values->push_back(min);

        col_avg.values.emplace_back();
        col_avg.numeric_values->push_back(avg);

        col_stdev.values.emplace_back();
        col_stdev.numeric_values->push_back(stdev);

        col_max.values.emplace_back();
        col_max.numeric_values->push_back(max);
    }

    graph->loaded_nodes[node.id].cache[0] = col_new_x;
    graph->loaded_nodes[node.id].cache[1] = col_min;
    graph->loaded_nodes[node.id].cache[2] = col_avg;
    graph->loaded_nodes[node.id].cache[3] = col_stdev;
    graph->loaded_nodes[node.id].cache[4] = col_max;

    switch (id) {
    default:
    case 0:
        return col_new_x;
    case 1:
        return col_min;
    case 2:
        return col_avg;
    case 3:
        return col_stdev;
    case 4:
        return col_max;
    }
}

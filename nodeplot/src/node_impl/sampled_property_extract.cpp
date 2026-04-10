#include "nodeplot.h"

#include <string_view>
#include <utility>
#include <vector>

template <>
ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const SampledPropertyExtractNode& node) {
    auto x = TRY_OR(graph->column_as_numeric(TRY_OR(graph->get_input(id.graph_name, node.i_x), return ERR("Could not get 'X' input"))), return ERR("'X' Column is not numeric"));
    auto y = TRY_OR(graph->column_as_numeric(TRY_OR(graph->get_input(id.graph_name, node.i_y), return ERR("Could not get 'Y' input"))), return ERR("'Y' Column is not numeric"));

    struct Bucket {
        std::vector<double> vals;
        double x_val;
    };

    std::map<double, Bucket> bucketed_vals_map;

    for (size_t i = 0; i < x.values.size(); i++) {
        double v = x.values[i];
        if (!bucketed_vals_map.contains(v)) {
            bucketed_vals_map.insert({v,
                                      Bucket{
                                          {},
                                          v,
                                      }});
        }
        bucketed_vals_map[v].vals.push_back(y.values[i]);
    }

    std::vector<Bucket> bucketed_vals;
    for (auto& b : bucketed_vals_map)
        bucketed_vals.push_back(b.second);
    std::sort(bucketed_vals.begin(), bucketed_vals.end(), [](Bucket& l, Bucket& r) { return l.x_val < r.x_val; });

    ColumnNumeric col_new_x;
    ColumnNumeric col_min;
    ColumnNumeric col_avg;
    ColumnNumeric col_stdev;
    ColumnNumeric col_max;
    ColumnNumeric col_sample_count;

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

        col_new_x.values.push_back(bucket.x_val);
        col_min.values.push_back(min);
        col_avg.values.push_back(avg);
        col_stdev.values.push_back(stdev);
        col_max.values.push_back(max);
        col_sample_count.values.push_back(bucket.vals.size());
    }

    return std::map<OutputIndex, NodeOutput>{
        {"x", Column{col_new_x}},
        {"min", Column{col_min}},
        {"avg", Column{col_avg}},
        {"stdev", Column{col_stdev}},
        {"max", Column{col_max}},
        {"sample_count", Column{col_sample_count}},
    };
}

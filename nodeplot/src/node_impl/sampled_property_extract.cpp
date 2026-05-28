#include "nodeplot.h"

#include <string_view>
#include <utility>
#include <vector>

using namespace NodePlot;

void register_sampled_property_extract() {
    NodeRegistry::register_node("sampled_property_extract",
                                Node{
                                    .type_id = "sampled_property_extract",
                                    .display_name = "Sampled Property Extract",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                            {"y", Node::Input{.id = "y", .display_name = "Y", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"x_vals", Node::Output{.id = "x_vals", .display_name = "X Vals", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                            {"min", Node::Output{.id = "min", .display_name = "Minimum", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                            {"avg", Node::Output{.id = "avg", .display_name = "Average", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                            {"stdev", Node::Output{.id = "stdev", .display_name = "Standard Deviation", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                            {"max", Node::Output{.id = "max", .display_name = "Maximum", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                            {"sample_count", Node::Output{.id = "sample_count", .display_name = "Sample Count", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        std::vector<double> x = TRY(eng->get_input_value<std::vector<double>>(npf, node_id, "x"));
                                        std::vector<double> y = TRY(eng->get_input_value<std::vector<double>>(npf, node_id, "y"));

                                        struct Bucket {
                                            std::vector<double> vals;
                                            double x_val;
                                        };

                                        std::map<double, Bucket> bucketed_vals_map;

                                        for (size_t i = 0; i < x.size(); i++) {
                                            double v = x[i];
                                            if (!bucketed_vals_map.contains(v)) {
                                                bucketed_vals_map.insert({v,
                                                                          Bucket{
                                                                              .vals = {},
                                                                              .x_val = v,
                                                                          }});
                                            }
                                            bucketed_vals_map[v].vals.push_back(y[i]);
                                        }

                                        std::vector<Bucket> bucketed_vals;
                                        for (auto& b : bucketed_vals_map)
                                            bucketed_vals.push_back(b.second);
                                        std::sort(bucketed_vals.begin(), bucketed_vals.end(), [](Bucket& l, Bucket& r) { return l.x_val < r.x_val; });

                                        std::vector<double> col_new_x;
                                        std::vector<double> col_min;
                                        std::vector<double> col_avg;
                                        std::vector<double> col_stdev;
                                        std::vector<double> col_max;
                                        std::vector<double> col_sample_count;

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

                                            col_new_x.push_back(bucket.x_val);
                                            col_min.push_back(min);
                                            col_avg.push_back(avg);
                                            col_stdev.push_back(stdev);
                                            col_max.push_back(max);
                                            col_sample_count.push_back(bucket.vals.size());
                                        }

                                        cache.computed_outputs["x_vals"] = col_new_x;
                                        cache.computed_outputs["min"] = col_min;
                                        cache.computed_outputs["avg"] = col_avg;
                                        cache.computed_outputs["stdev"] = col_stdev;
                                        cache.computed_outputs["max"] = col_max;
                                        cache.computed_outputs["sample_count"] = col_sample_count;

                                        return {};
                                    },
                                });

    NodeRegistry::register_node("distribution_properties",
                                Node{
                                    .type_id = "distribution_properties",
                                    .display_name = "Distribution Properties",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"values", Node::Input{.id = "values", .display_name = "Values", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"min", Node::Output{.id = "min", .display_name = "Minimum", .valid_data_types = {DataType::NUMBER}}},
                                            {"avg", Node::Output{.id = "avg", .display_name = "Average", .valid_data_types = {DataType::NUMBER}}},
                                            {"stdev", Node::Output{.id = "stdev", .display_name = "Standard Deviation", .valid_data_types = {DataType::NUMBER}}},
                                            {"max", Node::Output{.id = "max", .display_name = "Maximum", .valid_data_types = {DataType::NUMBER}}},
                                            {"sample_count", Node::Output{.id = "sample_count", .display_name = "Sample Count", .valid_data_types = {DataType::NUMBER}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        std::vector<double> v = TRY(eng->get_input_value<std::vector<double>>(npf, node_id, "values"));

                                        double sum = 0;
                                        double min = v.size() > 0 ? v.front() : 0.0;
                                        double max = v.size() > 0 ? v.front() : 0.0;

                                        for (auto& v : v) {
                                            sum += v;
                                            min = std::min(min, v);
                                            max = std::max(max, v);
                                        }

                                        double avg = sum / v.size();

                                        sum = 0;
                                        for (auto& v : v) {
                                            sum += std::pow(v - avg, 2);
                                        }
                                        double stdev = std::sqrt(sum / v.size());

                                        cache.computed_outputs["min"] = min;
                                        cache.computed_outputs["avg"] = max;
                                        cache.computed_outputs["stdev"] = stdev;
                                        cache.computed_outputs["max"] = max;
                                        cache.computed_outputs["sample_count"] = (double)v.size();

                                        return {};
                                    },
                                });
}

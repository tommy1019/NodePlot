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
                                            {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::COLUMN}}},
                                            {"y", Node::Input{.id = "y", .display_name = "Y", .valid_data_types = {DataType::COLUMN}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"x_vals", Node::Output{.id = "x_vals", .display_name = "X Vals", .valid_data_types = {DataType::COLUMN}}},
                                            {"min", Node::Output{.id = "min", .display_name = "Minimum", .valid_data_types = {DataType::COLUMN}}},
                                            {"avg", Node::Output{.id = "avg", .display_name = "Average", .valid_data_types = {DataType::COLUMN}}},
                                            {"stdev", Node::Output{.id = "stdev", .display_name = "Standard Deviation", .valid_data_types = {DataType::COLUMN}}},
                                            {"max", Node::Output{.id = "max", .display_name = "Maximum", .valid_data_types = {DataType::COLUMN}}},
                                            {"sample_count", Node::Output{.id = "sample_count", .display_name = "Sample Count", .valid_data_types = {DataType::COLUMN}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        Column::Numeric x = TRY(TRY(eng->get_input_value<Column>(npf, node_id, "x")).as_numeric_column());
                                        Column::Numeric y = TRY(TRY(eng->get_input_value<Column>(npf, node_id, "y")).as_numeric_column());

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
                                                                              .vals = {},
                                                                              .x_val = v,
                                                                          }});
                                            }
                                            bucketed_vals_map[v].vals.push_back(y.values[i]);
                                        }

                                        std::vector<Bucket> bucketed_vals;
                                        for (auto& b : bucketed_vals_map)
                                            bucketed_vals.push_back(b.second);
                                        std::sort(bucketed_vals.begin(), bucketed_vals.end(), [](Bucket& l, Bucket& r) { return l.x_val < r.x_val; });

                                        Column::Numeric col_new_x;
                                        Column::Numeric col_min;
                                        Column::Numeric col_avg;
                                        Column::Numeric col_stdev;
                                        Column::Numeric col_max;
                                        Column::Numeric col_sample_count;

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

                                        cache.computed_outputs["x_vals"] = Column{col_new_x};
                                        cache.computed_outputs["min"] = Column{col_min};
                                        cache.computed_outputs["avg"] = Column{col_avg};
                                        cache.computed_outputs["stdev"] = Column{col_stdev};
                                        cache.computed_outputs["max"] = Column{col_max};
                                        cache.computed_outputs["sample_count"] = Column{col_sample_count};

                                        return {};
                                    },
                                });
}

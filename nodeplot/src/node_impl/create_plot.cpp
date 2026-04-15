#include "nodeplot.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace NodePlot;

void register_create_plot() {
    NodeRegistry::register_node("create_plot_style",
                                Node{
                                    .type_id = "create_plot_style",
                                    .display_name = "Create Plot Style",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"plot_margines",
                                             Node::Input{
                                                 .id = "plot_margines",
                                                 .display_name = "Plot Margines",
                                                 .valid_data_types = {DataType::MARGINES},
                                                 .default_value = Margins{.left = 20, .right = 20, .top = 20, .bottom = 20},
                                             }},
                                            {"internal_plot_margines",
                                             Node::Input{
                                                 .id = "internal_plot_margines",
                                                 .display_name = "Internal Plot Margines",
                                                 .valid_data_types = {DataType::MARGINES},
                                                 .default_value = Margins{.left = 20, .right = 20, .top = 20, .bottom = 20},
                                             }},
                                            {"title_font_size",
                                             Node::Input{
                                                 .id = "title_font_size",
                                                 .display_name = "Title Font Size",
                                                 .valid_data_types = {DataType::NUMBER},
                                                 .default_value = 16.0,
                                             }},
                                            {"x_axis_tick_mark_font_size",
                                             Node::Input{
                                                 .id = "x_axis_tick_mark_font_size",
                                                 .display_name = "X Tick Mark Font Size",
                                                 .valid_data_types = {DataType::NUMBER},
                                                 .default_value = 12.0,
                                             }},
                                            {"x_axis_tick_mark_size",
                                             Node::Input{
                                                 .id = "x_axis_tick_mark_size",
                                                 .display_name = "X Tick Mark Size",
                                                 .valid_data_types = {DataType::NUMBER},
                                                 .default_value = 5.0,
                                             }},
                                            {"x_axis_label_font_size",
                                             Node::Input{
                                                 .id = "x_axis_label_font_size",
                                                 .display_name = "X Label Font Size",
                                                 .valid_data_types = {DataType::NUMBER},
                                                 .default_value = 12.0,
                                             }},
                                            {"y_axis_tick_mark_font_size",
                                             Node::Input{
                                                 .id = "y_axis_tick_mark_font_size",
                                                 .display_name = "Y Tick Mark Font Size",
                                                 .valid_data_types = {DataType::NUMBER},
                                                 .default_value = 12.0,
                                             }},
                                            {"y_axis_tick_mark_size",
                                             Node::Input{
                                                 .id = "y_axis_tick_mark_size",
                                                 .display_name = "Y Tick Mark Size",
                                                 .valid_data_types = {DataType::NUMBER},
                                                 .default_value = 5.0,
                                             }},
                                            {"y_axis_label_font_size",
                                             Node::Input{
                                                 .id = "y_axis_label_font_size",
                                                 .display_name = "Y Label Font Size",
                                                 .valid_data_types = {DataType::NUMBER},
                                                 .default_value = 12.0,
                                             }},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"style", Node::Output{.id = "style", .display_name = "Plot Style", .valid_data_types = {DataType::PLOT_STYLE}}},
                                        };
                                    },
                                    .evalulate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, NodeOutputCache& cache) -> ErrorOr<void> {
                                        PlotStyle res;
                                        res.plot_margines = TRY(eng->get_input_value<Margins>(npf, node_id, "plot_margines"));
                                        res.internal_plot_margines = TRY(eng->get_input_value<Margins>(npf, node_id, "internal_plot_margines"));

                                        res.title_font_size = TRY(eng->get_input_value<double>(npf, node_id, "title_font_size"));

                                        res.x_axis_tick_mark_font_size = TRY(eng->get_input_value<double>(npf, node_id, "x_axis_tick_mark_font_size"));
                                        res.x_axis_tick_mark_size = TRY(eng->get_input_value<double>(npf, node_id, "x_axis_tick_mark_size"));
                                        res.x_axis_label_font_size = TRY(eng->get_input_value<double>(npf, node_id, "x_axis_label_font_size"));

                                        res.y_axis_tick_mark_font_size = TRY(eng->get_input_value<double>(npf, node_id, "y_axis_tick_mark_font_size"));
                                        res.y_axis_tick_mark_size = TRY(eng->get_input_value<double>(npf, node_id, "y_axis_tick_mark_size"));
                                        res.y_axis_label_font_size = TRY(eng->get_input_value<double>(npf, node_id, "y_axis_label_font_size"));

                                        cache.computed_outputs["style"] = res;

                                        return {};
                                    },
                                });

    NodeRegistry::register_node("create_plot",
                                Node{
                                    .type_id = "create_plot",
                                    .display_name = "Create Plot",
                                    .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                                        std::vector<std::pair<InputId, Node::Input>> res;

                                        res.emplace_back("num_series", Node::Input{.id = "num_series", .display_name = "Number of Series", .valid_data_types = {DataType::INTEGER}});

                                        res.emplace_back("title", Node::Input{.id = "title", .display_name = "Title", .valid_data_types = {DataType::STRING}});
                                        res.emplace_back("x_label", Node::Input{.id = "x_label", .display_name = "X Label", .valid_data_types = {DataType::STRING}});
                                        res.emplace_back("y_label", Node::Input{.id = "y_label", .display_name = "Y Label", .valid_data_types = {DataType::STRING}});
                                        res.emplace_back("x_axis_log_scale", Node::Input{.id = "x_axis_tick_mark_size", .display_name = "X Axis Log Scale", .valid_data_types = {DataType::BOOLEAN}});
                                        res.emplace_back("y_axis_log_scale", Node::Input{.id = "y_axis_log_scale", .display_name = "X Axis Log Scale", .valid_data_types = {DataType::BOOLEAN}});
                                        res.emplace_back("style", Node::Input{.id = "style", .display_name = "Style", .valid_data_types = {DataType::PLOT_STYLE}});

                                        int64_t num_series = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "num_series").value_or(0), int64_t{0}, int64_t{255});
                                        for (int64_t i = 0; i < num_series; i++) {
                                            std::string name = "series_" + std::to_string(i);
                                            res.emplace_back(name, Node::Input{.id = name, .display_name = "Series " + std::to_string(i), .valid_data_types = {DataType::SERIES}});
                                        }

                                        return res;
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"plot", Node::Output{.id = "plot", .display_name = "Plot", .valid_data_types = {DataType::PLOT}}},
                                        };
                                    },
                                    .evalulate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, NodeOutputCache& cache) -> ErrorOr<void> {
                                        Plot res;
                                        res.title = TRY(eng->get_input_value<std::string>(npf, node_id, "title"));

                                        res.x_label = TRY(eng->get_input_value<std::string>(npf, node_id, "x_label"));
                                        res.y_label = TRY(eng->get_input_value<std::string>(npf, node_id, "y_label"));

                                        res.x_axis_log_scale = TRY(eng->get_input_value<bool>(npf, node_id, "x_axis_log_scale"));
                                        res.y_axis_log_scale = TRY(eng->get_input_value<bool>(npf, node_id, "y_axis_log_scale"));

                                        res.style = TRY(eng->get_input_value<PlotStyle>(npf, node_id, "style"));

                                        int64_t num_series = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "num_series").value_or(0), int64_t{0}, int64_t{255});
                                        for (int64_t i = 0; i < num_series; i++) {
                                            res.series.push_back(TRY(eng->get_input_value<Series>(npf, node_id, "series_" + std::to_string(i))));
                                        }

                                        cache.computed_outputs["plot"] = res;

                                        return {};
                                    },
                                });
}

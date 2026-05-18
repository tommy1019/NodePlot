#include "nodeplot.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace NodePlot;

void register_create_plot_style() {
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
                                                 .default_value = Margins{.left = 0.16f, .right = 0.05f, .top = 0.14f, .bottom = 0.15f},
                                             }},
                                            {"internal_plot_margines",
                                             Node::Input{
                                                 .id = "internal_plot_margines",
                                                 .display_name = "Internal Plot Margines",
                                                 .valid_data_types = {DataType::MARGINES},
                                                 .default_value = Margins{.left = 0, .right = 0, .top = 0, .bottom = 0},
                                             }},
                                            {"title_font_size",
                                             Node::Input{
                                                 .id = "title_font_size",
                                                 .display_name = "Title Font Size",
                                                 .valid_data_types = {DataType::NUMBER},
                                                 .default_value = 16.0,
                                             }},
                                            {"title_offset",
                                             Node::Input{
                                                 .id = "title_offset",
                                                 .display_name = "Title Offset",
                                                 .valid_data_types = {DataType::POSITION},
                                                 .default_value = Pos{0.0f, 0.1f},
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
                                                 .default_value = 0.02f,
                                             }},
                                            {"x_axis_tick_mark_offset",
                                             Node::Input{
                                                 .id = "x_axis_tick_mark_offset",
                                                 .display_name = "X Tick Offset",
                                                 .valid_data_types = {DataType::POSITION},
                                                 .default_value = Pos{0.0f, 0.04f},
                                             }},
                                            {"x_axis_label_font_size",
                                             Node::Input{
                                                 .id = "x_axis_label_font_size",
                                                 .display_name = "X Label Font Size",
                                                 .valid_data_types = {DataType::NUMBER},
                                                 .default_value = 12.0,
                                             }},
                                            {"x_axis_label_offset",
                                             Node::Input{
                                                 .id = "x_axis_label_offset",
                                                 .display_name = "X Label Offset",
                                                 .valid_data_types = {DataType::POSITION},
                                                 .default_value = Pos{0.0f, 0.0f},
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
                                                 .default_value = 0.02,
                                             }},
                                            {"y_axis_tick_mark_offset",
                                             Node::Input{
                                                 .id = "y_axis_tick_mark_offset",
                                                 .display_name = "Y Tick Offset",
                                                 .valid_data_types = {DataType::POSITION},
                                                 .default_value = Pos{-0.01f, 0.015f},
                                             }},
                                            {"y_axis_label_font_size",
                                             Node::Input{
                                                 .id = "y_axis_label_font_size",
                                                 .display_name = "Y Label Font Size",
                                                 .valid_data_types = {DataType::NUMBER},
                                                 .default_value = 12.0,
                                             }},
                                            {"y_axis_label_offset",
                                             Node::Input{
                                                 .id = "y_axis_label_offset",
                                                 .display_name = "Y Label Offset",
                                                 .valid_data_types = {DataType::POSITION},
                                                 .default_value = Pos{0.06f, 0.0f},
                                             }},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"style", Node::Output{.id = "style", .display_name = "Plot Style", .valid_data_types = {DataType::PLOT_STYLE}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        PlotStyle res;
                                        res.plot_margines = TRY(eng->get_input_value<Margins>(npf, node_id, "plot_margines"));
                                        res.internal_plot_margines = TRY(eng->get_input_value<Margins>(npf, node_id, "internal_plot_margines"));

                                        res.title_font_size = TRY(eng->get_input_value<double>(npf, node_id, "title_font_size"));
                                        res.title_offset = TRY(eng->get_input_value<Pos>(npf, node_id, "title_offset"));

                                        res.x_axis_tick_mark_font_size = TRY(eng->get_input_value<double>(npf, node_id, "x_axis_tick_mark_font_size"));
                                        res.x_axis_tick_mark_size = TRY(eng->get_input_value<double>(npf, node_id, "x_axis_tick_mark_size"));
                                        res.x_axis_tick_mark_offset = TRY(eng->get_input_value<Pos>(npf, node_id, "x_axis_tick_mark_offset"));
                                        res.x_axis_label_font_size = TRY(eng->get_input_value<double>(npf, node_id, "x_axis_label_font_size"));
                                        res.x_axis_label_offset = TRY(eng->get_input_value<Pos>(npf, node_id, "x_axis_label_offset"));

                                        res.y_axis_tick_mark_font_size = TRY(eng->get_input_value<double>(npf, node_id, "y_axis_tick_mark_font_size"));
                                        res.y_axis_tick_mark_size = TRY(eng->get_input_value<double>(npf, node_id, "y_axis_tick_mark_size"));
                                        res.y_axis_tick_mark_offset = TRY(eng->get_input_value<Pos>(npf, node_id, "y_axis_tick_mark_offset"));
                                        res.y_axis_label_font_size = TRY(eng->get_input_value<double>(npf, node_id, "y_axis_label_font_size"));
                                        res.y_axis_label_offset = TRY(eng->get_input_value<Pos>(npf, node_id, "y_axis_label_offset"));

                                        cache.computed_outputs["style"] = res;

                                        return {};
                                    },
                                });
}

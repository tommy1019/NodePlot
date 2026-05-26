#include "nodeplot.h"

using namespace NodePlot;

void register_value() {
    NodeRegistry::register_node("numeric_value",
                                Node{
                                    .type_id = "numeric_value",
                                    .display_name = "Numeric Value",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"value", Node::Input{.id = "value", .display_name = "Value", .valid_data_types = {DataType::NUMBER}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"value", Node::Output{.id = "value", .display_name = "Value", .valid_data_types = {DataType::NUMBER}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        cache.computed_outputs["value"] = TRY(eng->get_input_value<double>(npf, node_id, "value"));
                                        return {};
                                    },
                                });

    NodeRegistry::register_node("string_value",
                                Node{
                                    .type_id = "string_value",
                                    .display_name = "String Value",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"value", Node::Input{.id = "value", .display_name = "Value", .valid_data_types = {DataType::STRING}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"value", Node::Output{.id = "value", .display_name = "Value", .valid_data_types = {DataType::STRING}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        cache.computed_outputs["value"] = TRY(eng->get_input_value<std::string>(npf, node_id, "value"));
                                        return {};
                                    },
                                });

    NodeRegistry::register_node("color_value",
                                Node{
                                    .type_id = "color_value",
                                    .display_name = "Color Value",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"value", Node::Input{.id = "value", .display_name = "Value", .valid_data_types = {DataType::COLOR}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"value", Node::Output{.id = "value", .display_name = "Value", .valid_data_types = {DataType::COLOR}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        cache.computed_outputs["value"] = TRY(eng->get_input_value<Color>(npf, node_id, "value"));
                                        return {};
                                    },
                                });

    NodeRegistry::register_node("color_compose",
                                Node{
                                    .type_id = "color_compose",
                                    .display_name = "Color Compose",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"red", Node::Input{.id = "red", .display_name = "Red", .valid_data_types = {DataType::NUMBER}}},
                                            {"green", Node::Input{.id = "green", .display_name = "Green", .valid_data_types = {DataType::NUMBER}}},
                                            {"blue", Node::Input{.id = "blue", .display_name = "Blue", .valid_data_types = {DataType::NUMBER}}},
                                            {"alpha", Node::Input{.id = "alpha", .display_name = "alpha", .valid_data_types = {DataType::NUMBER}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"color", Node::Output{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        cache.computed_outputs["color"] = Color{
                                            .r = (float)TRY(eng->get_input_value<double>(npf, node_id, "red")),
                                            .g = (float)TRY(eng->get_input_value<double>(npf, node_id, "green")),
                                            .b = (float)TRY(eng->get_input_value<double>(npf, node_id, "blue")),
                                            .a = (float)TRY(eng->get_input_value<double>(npf, node_id, "alpha")),
                                        };
                                        return {};
                                    },
                                });
    NodeRegistry::register_node("color_decompose",
                                Node{
                                    .type_id = "color_decompose",
                                    .display_name = "Color Decompose",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"red", Node::Output{.id = "red", .display_name = "Red", .valid_data_types = {DataType::NUMBER}}},
                                            {"green", Node::Output{.id = "green", .display_name = "Green", .valid_data_types = {DataType::NUMBER}}},
                                            {"blue", Node::Output{.id = "blue", .display_name = "Blue", .valid_data_types = {DataType::NUMBER}}},
                                            {"alpha", Node::Output{.id = "alpha", .display_name = "alpha", .valid_data_types = {DataType::NUMBER}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        Color c = TRY(eng->get_input_value<Color>(npf, node_id, "color"));
                                        cache.computed_outputs["red"] = c.r;
                                        cache.computed_outputs["green"] = c.g;
                                        cache.computed_outputs["blue"] = c.b;
                                        cache.computed_outputs["alpha"] = c.a;
                                        return {};
                                    },
                                });
}

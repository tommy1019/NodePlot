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
}

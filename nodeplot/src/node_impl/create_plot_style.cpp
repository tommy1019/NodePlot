#include "error.h"
#include "nodeplot.h"
#include "types.h"
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
                                            {"style",
                                             Node::Input{
                                                 .id = "style",
                                                 .display_name = "Style",
                                                 .valid_data_types = {DataType::PLOT_STYLE},
                                             }},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"style", Node::Output{.id = "style", .display_name = "Plot Style", .valid_data_types = {DataType::PLOT_STYLE}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        cache.computed_outputs["style"] = TRY(eng->get_input_style(npf, node_id, "style"));
                                        return {};
                                    },
                                });
}

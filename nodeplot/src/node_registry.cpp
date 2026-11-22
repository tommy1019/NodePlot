#include "node_registry.h"

#include "eval_node_graph.h"
#include <expected>

extern void register_output();

extern void register_binary_operation();
extern void register_column_select();
extern void register_create_plot_style();
extern void register_csv_import();
extern void register_figure();
extern void register_filter_table();
extern void register_function();
extern void register_grid_layout();
extern void register_sampled_property_extract();
extern void register_series_create();
extern void register_value();

namespace NodePlot {

std::map<NodeTypeId, Node> NodeRegistry::node_map = {};
std::map<SeriesTypeId, SeriesDefinition> NodeRegistry::series_map = {};

void NodeRegistry::init() {
    register_binary_operation();
    register_column_select();
    register_create_plot_style();
    register_csv_import();
    register_figure();
    register_filter_table();
    register_function();
    register_grid_layout();
    register_sampled_property_extract();
    register_series_create();
    register_value();
    register_output();
}

void NodeRegistry::NodeRegistry::register_node(NodeTypeId node_id, Node node) { NodeRegistry::node_map.insert({node_id, node}); }

void NodeRegistry::register_series(SeriesTypeId series_id, std::vector<std::string> toolbar_path, Node::InputGenerator input_generator, SeriesDefinition def) {
    register_node("series_create_" + series_id,
                  Node{
                      .type_id = "series_create_" + series_id,
                      .display_name = "Create " + def.display_name + " Series",
                      .toolbar_path = toolbar_path,
                      .inputs = input_generator,
                      .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                          return {
                              {"series", Node::Output{.id = "series", .display_name = "Series", .valid_data_types = {DataType::SERIES}}},
                          };
                      },
                      .evaluate = [series_id, input_generator](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                          auto inputs_or_err = input_generator(npf, eng, node_id);
                          if (!inputs_or_err.has_value()) {
                              return std::unexpected(Node::EvaluationError{
                                  .description = "Failed to get series input definition",
                                  .input_errors = {},
                              });
                          }

                          GenericSeries res;
                          res.type_id = series_id;

                          for (auto [id, input] : inputs_or_err.value()) {
                              auto data = TRY(eng->get_input_data(npf, node_id, id, true));
                              res.data[id] = data;
                          }

                          output["series"] = res;

                          return {};
                      },
                  });

    series_map[series_id] = def;
}
} // namespace NodePlot

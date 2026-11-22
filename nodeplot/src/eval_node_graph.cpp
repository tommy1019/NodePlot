#include "eval_node_graph.h"

#include "node_registry.h"
#include "nodeplot.h"
#include <expected>

namespace NodePlot {

ErrorOr<std::reference_wrapper<NodeGraph>> EvaluatedNodeGraph::node_graph(NodePlotFile* npf) {
    auto ng = npf->graphs.find(graph_id);
    if (ng == npf->graphs.end())
        return ERR("Invalid graph id");
    return ng->second;
}

Node::EvaluateData EvaluatedNodeGraph::get_output_data(NodePlotFile* npf, NodeId node_id, OutputId output_id) {
    OutputCache& output_cache = cache[node_id];

    auto data = output_cache.computed_outputs.find(output_id);
    if (data != output_cache.computed_outputs.end()) {
        return data->second;
    }

    auto ng = npf->graphs.find(this->graph_id);
    if (ng == npf->graphs.end()) {
        output_cache.error = Node::EvaluationError{
            .description = "Invalid graph id",
            .input_errors = {},
        };
        return std::unexpected(output_cache.error.value());
    }

    auto node_storage = ng->second.nodes.find(node_id);
    if (node_storage == ng->second.nodes.end()) {
        output_cache.error = Node::EvaluationError{
            .description = "Invalid node id",
            .input_errors = {},
        };
        return std::unexpected(output_cache.error.value());
    }

    auto node = NodeRegistry::node_map.find(node_storage->second.type_id);
    if (node == NodeRegistry::node_map.end()) {
        output_cache.error = Node::EvaluationError{
            .description = "Invalid node type id",
            .input_errors = {},
        };
        return std::unexpected(output_cache.error.value());
    }

    auto eval_err = node->second.evaluate(npf, this, node_id, output_cache.computed_outputs);
    if (!eval_err.has_value()) {
        output_cache.error = eval_err.error();
        return std::unexpected(output_cache.error.value());
    }

    auto res = output_cache.computed_outputs.find(output_id);
    if (res == output_cache.computed_outputs.end()) {
        return std::unexpected(Node::EvaluationError{
            .description = "Invalid output id",
            .input_errors = {},
        });
    }

    return res->second;
}

Node::EvaluateData EvaluatedNodeGraph::get_input_data(NodePlotFile* npf, NodeId node_id, InputId input_id, bool fill_default_value) {
    auto ng = npf->graphs.find(this->graph_id);
    if (ng == npf->graphs.end()) {
        return std::unexpected(Node::EvaluationError{
            .description = "Invalid graph id",
            .input_errors = {},
        });
    }

    auto node_storage = ng->second.nodes.find(node_id);
    if (node_storage == ng->second.nodes.end()) {
        return std::unexpected(Node::EvaluationError{
            .description = "Invalid node id",
            .input_errors = {},
        });
    }

    auto input_storage = node_storage->second.input_storage.find(input_id);
    if (input_storage == node_storage->second.input_storage.end()) {
        if (fill_default_value) {

            auto node = NodeRegistry::node_map.find(node_storage->second.type_id);
            if (node == NodeRegistry::node_map.end()) {
                return std::unexpected(Node::EvaluationError{
                    .description = "Invalid node type id",
                    .input_errors = {input_id},
                });
            }

            auto inputs = node->second.inputs(npf, this, node_id);
            if (!inputs.has_value()) {
                return std::unexpected(Node::EvaluationError{
                    .description = "Could not get inputs for this node_type " + node_storage->second.type_id,
                    .input_errors = {input_id},
                });
            }

            for (auto& input : inputs.value()) {
                if (input.first == input_id) {
                    if (input.second.default_value.has_value())
                        return input.second.default_value.value();
                    break;
                }
            }

            return std::unexpected(Node::EvaluationError{
                .description = "Node has no value for '" + input_id + "' and no default value",
                .input_errors = {input_id},
            });

        } else {
            return std::unexpected(Node::EvaluationError{
                .description = "Node has no value for '" + input_id + "'",
                .input_errors = {input_id},
            });
        }
    }

    return std::visit(Utils::overloaded{
                          [&](Data data) -> std::expected<Data, Node::EvaluationError> { return data; },
                          [&](InputPin input_pin) -> std::expected<Data, Node::EvaluationError> {
                              auto res = this->get_output_data(npf, input_pin.node_id, input_pin.output_id);
                              if (res.has_value())
                                  return res;

                              return std::unexpected(Node::EvaluationError{
                                  .description = "Could not get input for '" + input_id + "'",
                                  .input_errors = {input_id},
                              });
                          },
                      },
                      input_storage->second);
}

std::expected<PlotStyle, Node::EvaluationError> EvaluatedNodeGraph::get_input_style(NodePlotFile* npf, NodeId node_id, InputId id) {

    auto ng = npf->graphs.find(this->graph_id);
    if (ng == npf->graphs.end()) {
        return std::unexpected(Node::EvaluationError{
            .description = "Invalid graph id",
            .input_errors = {},
        });
    }

    auto node_storage = ng->second.nodes.find(node_id);
    if (node_storage == ng->second.nodes.end()) {
        return std::unexpected(Node::EvaluationError{
            .description = "Invalid node id",
            .input_errors = {},
        });
    }

    auto& str = node_storage->second.input_storage;

    PlotStyle res = DEFAULT_PLOT_STYLE;

    if (str.contains(id + "_base")) {
        res = TRY(get_input_value<PlotStyle>(npf, node_id, id + "_base", false));
    }

    if (str.contains(id + "_plot_margins"))
        res.plot_margins = TRY(get_input_value<Margins>(npf, node_id, id + "_plot_margins", false));
    if (str.contains(id + "_internal_plot_margins"))
        res.internal_plot_margins = TRY(get_input_value<Margins>(npf, node_id, id + "_internal_plot_margins", false));

    if (str.contains(id + "_title_font_size"))
        res.title_font_size = TRY(get_input_value<double>(npf, node_id, id + "_title_font_size", false));
    if (str.contains(id + "_title_offset"))
        res.title_offset = TRY(get_input_value<Pos>(npf, node_id, id + "_title_offset", false));

    if (str.contains(id + "_x_axis_stroke_width"))
        res.x_axis_stroke_width = TRY(get_input_value<double>(npf, node_id, id + "_x_axis_stroke_width", false));
    if (str.contains(id + "_x_axis_tick_mark_font_size"))
        res.x_axis_tick_mark_font_size = TRY(get_input_value<double>(npf, node_id, id + "_x_axis_tick_mark_font_size", false));
    if (str.contains(id + "_x_axis_tick_mark_size"))
        res.x_axis_tick_mark_size = TRY(get_input_value<double>(npf, node_id, id + "_x_axis_tick_mark_size", false));
    if (str.contains(id + "_x_axis_tick_mark_stroke_width"))
        res.x_axis_tick_mark_stroke_width = TRY(get_input_value<double>(npf, node_id, id + "_x_axis_tick_mark_stroke_width", false));
    if (str.contains(id + "_x_axis_tick_mark_offset"))
        res.x_axis_tick_mark_offset = TRY(get_input_value<Pos>(npf, node_id, id + "_x_axis_tick_mark_offset", false));
    if (str.contains(id + "_x_axis_label_font_size"))
        res.x_axis_label_font_size = TRY(get_input_value<double>(npf, node_id, id + "_x_axis_label_font_size", false));
    if (str.contains(id + "_x_axis_label_offset"))
        res.x_axis_label_offset = TRY(get_input_value<Pos>(npf, node_id, id + "_x_axis_label_offset", false));

    if (str.contains(id + "_y_axis_stroke_width"))
        res.y_axis_stroke_width = TRY(get_input_value<double>(npf, node_id, id + "_y_axis_stroke_width", false));
    if (str.contains(id + "_y_axis_tick_mark_font_size"))
        res.y_axis_tick_mark_font_size = TRY(get_input_value<double>(npf, node_id, id + "_y_axis_tick_mark_font_size", false));
    if (str.contains(id + "_y_axis_tick_mark_size"))
        res.y_axis_tick_mark_size = TRY(get_input_value<double>(npf, node_id, id + "_y_axis_tick_mark_size", false));
    if (str.contains(id + "_y_axis_tick_mark_stroke_width"))
        res.y_axis_tick_mark_stroke_width = TRY(get_input_value<double>(npf, node_id, id + "_y_axis_tick_mark_stroke_width", false));
    if (str.contains(id + "_y_axis_tick_mark_offset"))
        res.y_axis_tick_mark_offset = TRY(get_input_value<Pos>(npf, node_id, id + "_y_axis_tick_mark_offset", false));
    if (str.contains(id + "_y_axis_label_font_size"))
        res.y_axis_label_font_size = TRY(get_input_value<double>(npf, node_id, id + "_y_axis_label_font_size", false));
    if (str.contains(id + "_y_axis_label_offset"))
        res.y_axis_label_offset = TRY(get_input_value<Pos>(npf, node_id, id + "_y_axis_label_offset", false));

    return res;
}

} // namespace NodePlot
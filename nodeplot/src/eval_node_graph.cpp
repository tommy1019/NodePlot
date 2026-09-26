#include "eval_node_graph.h"

#include "nodeplot.h"

namespace NodePlot {

ErrorOr<std::reference_wrapper<NodeGraph>> EvaluatedNodeGraph::node_graph(NodePlotFile* npf) {
    auto ng = npf->graphs.find(graph_id);
    if (ng == npf->graphs.end())
        return ERR("Invalid graph id");
    return ng->second;
}

ErrorOr<Data> EvaluatedNodeGraph::get_output_data(NodePlotFile* npf, NodeId node_id, OutputId output_id) {
    OutputCache& output_cache = cache[node_id];

    auto data = output_cache.computed_outputs.find(output_id);
    if (data != output_cache.computed_outputs.end()) {
        return data->second;
    }

    auto& ng = TRY(node_graph(npf)).get();
    NodeGraph::NodeStorage& node_input_storage = TRY(Utils::try_find(ng.nodes, node_id, "Invalid NodeID")).get();
    Node& node = TRY(Utils::try_find(NodeRegistry::node_map, node_input_storage.type_id, "Invalid NodeTypeId")).get();

    auto eval_err = node.evaluate(npf, this, node_id, output_cache);
    if (!eval_err.has_value()) {
        output_cache.error = eval_err.error();
    }

    return TRY(Utils::try_find(output_cache.computed_outputs, output_id, "OutputId not found")).get();
}

ErrorOr<Data> EvaluatedNodeGraph::get_input_data(NodePlotFile* npf, NodeId node_id, InputId input_id, bool fill_default_value) {
    auto& ng = TRY(node_graph(npf)).get();
    NodeGraph::NodeStorage& node_storage = TRY(Utils::try_find(ng.nodes, node_id, "Invalid NodeID")).get();
    auto input_storage_or_error = Utils::try_find(node_storage.input_storage, input_id, "Invalid InputID: " + input_id);

    if (!input_storage_or_error.has_value()) {
        if (fill_default_value) {
            auto inputs = TRY(TRY(Utils::try_find(NodeRegistry::node_map, node_storage.type_id, "Invalid Node Type ID")).get().inputs(npf, this, node_id));

            for (auto& input : inputs) {
                if (input.first == input_id) {
                    if (input.second.default_value.has_value())
                        return input.second.default_value.value();
                    break;
                }
            }

            return ERR("Node has no value for " + input_id + " and no default value");
        } else {
            return ERR("Node has no value for " + input_id);
        }
    }

    return std::visit(Utils::overloaded{
                          [&](Data data) -> ErrorOr<Data> { return data; },
                          [&](InputPin input_pin) -> ErrorOr<Data> { return this->get_output_data(npf, input_pin.node_id, input_pin.output_id); },
                      },
                      input_storage_or_error.value().get());
}

ErrorOr<PlotStyle> EvaluatedNodeGraph::get_input_style(NodePlotFile* npf, NodeId node_id, InputId id) {
    auto& str = TRY(Utils::try_find(TRY(node_graph(npf)).get().nodes, node_id, "Invalid NodeID")).get().input_storage;

    PlotStyle res = DEFAULT_PLOT_STYLE;

    if (str.contains(id + "_base")) {
        res = TRY(get_input_value<PlotStyle>(npf, node_id, id + "_base", false));
    }

    if (str.contains(id + "_plot_margins"))
        res.plot_margins = TRY(get_input_value<Margins>(npf, node_id, id + "_plot_margins", false));
    if (str.contains(id + "_internal_plot_margins"))
        res.plot_margins = TRY(get_input_value<Margins>(npf, node_id, id + "_internal_plot_margins", false));

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
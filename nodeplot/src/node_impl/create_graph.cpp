#include "nodeplot.h"
#include <vector>

#define GET_INPUT(name, label) TRY_OR(graph->get_input(node.i_##name), return ERR("Could not get '" #label "' input"))

#define GET_INPUT_VAR(name, label) auto name = TRY_OR(graph->get_input(node.i_##name), return ERR("Could not get '" #label "' input"));

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const CreateGraphStyleNode& node, OutputId id) {
    GraphStyle res;
    res.plot_margines = GET_INPUT(plot_margines, Plot Margines);
    res.internal_plot_margines = GET_INPUT(internal_margines, Internal Plot Margines);

    res.x_axis_tick_mark_font_size = GET_INPUT(x_axis_tick_mark_font_size, X Axis Tick Mark Font Size);
    res.x_axis_tick_mark_size = GET_INPUT(x_axis_tick_mark_size, X Axis Tick Mark Size);
    res.x_axis_label_font_size = GET_INPUT(x_axis_label_font_size, X Axis Label Font Size);

    res.y_axis_tick_mark_font_size = GET_INPUT(y_axis_tick_mark_font_size, Y Axis Tick Mark Font Size);
    res.y_axis_tick_mark_size = GET_INPUT(y_axis_tick_mark_size, Y Axis Tick Mark Size);
    res.y_axis_label_font_size = GET_INPUT(y_axis_label_font_size, Y Axis Label Font Size);

    res.title_font_size = GET_INPUT(title_font_size, Title Font Size);

    return res;
}

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const CreateGraphNode& node, OutputId id) {
    GET_INPUT_VAR(title, Title)
    GET_INPUT_VAR(xlab, X Label)
    GET_INPUT_VAR(ylab, Y Label)
    GET_INPUT_VAR(x_axis_log_scale, X Log Scale)
    GET_INPUT_VAR(y_axis_log_scale, Y Log Scale)
    GET_INPUT_VAR(style, Style)

    std::vector<Series> series;
    for (auto& p : node.i_series) {
        auto series_or_error = graph->get_input(p);
        if (series_or_error.has_value())
            series.push_back(*series_or_error);
    }

    if (series.empty()) {
        return ERR("No valid series data");
    }

    Graph res;
    res.series = {series};
    res.title = title;
    res.x_lab = xlab;
    res.y_lab = ylab;
    res.x_log = x_axis_log_scale;
    res.y_log = y_axis_log_scale;
    res.style = style;

    return res;
}

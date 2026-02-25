#include "nodeplot.h"

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const ScatterSeriesCreateNode& node, OutputId id) {
    auto x = TRY_OR(graph->get_input(node.i_x), return ERR("Could not get 'X' input"));
    auto y = TRY_OR(graph->get_input(node.i_y), return ERR("Could not get 'Y' input"));
    auto color = TRY_OR(graph->get_input(node.i_color), return ERR("Could not get 'Color' input"));
    auto point_size = TRY_OR(graph->get_input(node.i_point_size), return ERR("Could not get 'Point Size' input"));

    Series res = ScatterSeries{.x = x, .y = y, .color = color, .point_size = point_size};
    return res;
}

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const LineSeriesCreateNode& node, OutputId id) {
    auto x = TRY_OR(graph->get_input(node.i_x), return ERR("Could not get 'X' input"));
    auto y = TRY_OR(graph->get_input(node.i_y), return ERR("Could not get 'Y' input"));
    auto color = TRY_OR(graph->get_input(node.i_color), return ERR("Could not get 'Color' input"));
    auto stroke_width = TRY_OR(graph->get_input(node.i_stroke_width), return ERR("Could not get 'Stroke Width' input"));

    Series res = LineSeries{.x = x, .y = y, .color = color, .stroke_width = stroke_width};
    return res;
}

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const RibbonSeriesCreateNode& node, OutputId id) {
    auto x = TRY_OR(graph->get_input(node.i_x), return ERR("Could not get 'X' input"));
    auto y_min = TRY_OR(graph->get_input(node.i_y_min), return ERR("Could not get 'Y Min' input"));
    auto y_max = TRY_OR(graph->get_input(node.i_y_max), return ERR("Could not get 'Y Max' input"));
    auto color = TRY_OR(graph->get_input(node.i_color), return ERR("Could not get 'Color' input"));

    Series res = RibbonSeries{.x = x, .y_min = y_min, .y_max = y_max, .color = color};
    return res;
}

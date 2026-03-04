#include "nodeplot.h"

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const ScatterSeriesCreateNode& node, OutputIndex id) {
    auto x = TRY_OR(graph->get_input(node.i_x), return ERR("Could not get 'X' input"));
    auto y = TRY_OR(graph->get_input(node.i_y), return ERR("Could not get 'Y' input"));
    auto color = TRY_OR(graph->get_input(node.i_color), return ERR("Could not get 'Color' input"));
    auto point_size = TRY_OR(graph->get_input(node.i_point_size), return ERR("Could not get 'Point Size' input"));

    Series res = ScatterSeries{.x = TRY_OR(graph->column_as_numeric(x), return ERR("'X' Column is not numeric")),
                               .y = TRY_OR(graph->column_as_numeric(y), return ERR("'Y' Column is not numeric")),
                               .color = color,
                               .point_size = point_size};
    return res;
}

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const LineSeriesCreateNode& node, OutputIndex id) {
    auto x = TRY_OR(graph->get_input(node.i_x), return ERR("Could not get 'X' input"));
    auto y = TRY_OR(graph->get_input(node.i_y), return ERR("Could not get 'Y' input"));
    auto color = TRY_OR(graph->get_input(node.i_color), return ERR("Could not get 'Color' input"));
    auto stroke_width = TRY_OR(graph->get_input(node.i_stroke_width), return ERR("Could not get 'Stroke Width' input"));

    Series res = LineSeries{.x = TRY_OR(graph->column_as_numeric(x), return ERR("'X' Column is not numeric")),
                            .y = TRY_OR(graph->column_as_numeric(y), return ERR("'Y' Column is not numeric")),
                            .color = color,
                            .stroke_width = stroke_width};
    return res;
}

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const RibbonSeriesCreateNode& node, OutputIndex id) {
    auto x = TRY_OR(graph->get_input(node.i_x), return ERR("Could not get 'X' input"));
    auto y_min = TRY_OR(graph->get_input(node.i_y_min), return ERR("Could not get 'Y Min' input"));
    auto y_max = TRY_OR(graph->get_input(node.i_y_max), return ERR("Could not get 'Y Max' input"));
    auto color = TRY_OR(graph->get_input(node.i_color), return ERR("Could not get 'Color' input"));

    Series res = RibbonSeries{.x = TRY_OR(graph->column_as_numeric(x), return ERR("'X' Column is not numeric")),
                              .y_min = TRY_OR(graph->column_as_numeric(y_min), return ERR("'Y Min' Column is not numeric")),
                              .y_max = TRY_OR(graph->column_as_numeric(y_max), return ERR("'Y Max' Column is not numeric")),
                              .color = color};
    return res;
}

#include "nodeplot.h"

template <>
ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const ScatterSeriesCreateNode& node) {
    auto x = TRY_OR(graph->get_input(id.graph_name, node.i_x), return ERR("Could not get 'X' input"));
    auto y = TRY_OR(graph->get_input(id.graph_name, node.i_y), return ERR("Could not get 'Y' input"));
    auto color = TRY_OR(graph->get_input(id.graph_name, node.i_color), return ERR("Could not get 'Color' input"));
    auto point_size = TRY_OR(graph->get_input(id.graph_name, node.i_point_size), return ERR("Could not get 'Point Size' input"));

    Series res = ScatterSeries{.x = TRY_OR(graph->column_as_numeric(x), return ERR("'X' Column is not numeric")),
                               .y = TRY_OR(graph->column_as_numeric(y), return ERR("'Y' Column is not numeric")),
                               .color = color,
                               .point_size = point_size};
    return std::map<OutputIndex, NodeOutput>{{"series", res}};
}

template <>
ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const LineSeriesCreateNode& node) {
    auto x = TRY_OR(graph->get_input(id.graph_name, node.i_x), return ERR("Could not get 'X' input"));
    auto y = TRY_OR(graph->get_input(id.graph_name, node.i_y), return ERR("Could not get 'Y' input"));
    auto color = TRY_OR(graph->get_input(id.graph_name, node.i_color), return ERR("Could not get 'Color' input"));
    auto stroke_width = TRY_OR(graph->get_input(id.graph_name, node.i_stroke_width), return ERR("Could not get 'Stroke Width' input"));

    Series res = LineSeries{.x = TRY_OR(graph->column_as_numeric(x), return ERR("'X' Column is not numeric")),
                            .y = TRY_OR(graph->column_as_numeric(y), return ERR("'Y' Column is not numeric")),
                            .color = color,
                            .stroke_width = stroke_width};
    return std::map<OutputIndex, NodeOutput>{{"series", res}};
}

template <>
ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const RibbonSeriesCreateNode& node) {
    auto x = TRY_OR(graph->get_input(id.graph_name, node.i_x), return ERR("Could not get 'X' input"));
    auto y_min = TRY_OR(graph->get_input(id.graph_name, node.i_y_min), return ERR("Could not get 'Y Min' input"));
    auto y_max = TRY_OR(graph->get_input(id.graph_name, node.i_y_max), return ERR("Could not get 'Y Max' input"));
    auto color = TRY_OR(graph->get_input(id.graph_name, node.i_color), return ERR("Could not get 'Color' input"));

    Series res = RibbonSeries{.x = TRY_OR(graph->column_as_numeric(x), return ERR("'X' Column is not numeric")),
                              .y_min = TRY_OR(graph->column_as_numeric(y_min), return ERR("'Y Min' Column is not numeric")),
                              .y_max = TRY_OR(graph->column_as_numeric(y_max), return ERR("'Y Max' Column is not numeric")),
                              .color = color};
    return std::map<OutputIndex, NodeOutput>{{"series", res}};
}

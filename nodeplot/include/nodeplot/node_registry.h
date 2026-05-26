#pragma once

#include <functional>

#include "node.h"
#include "types.h"

namespace NodePlot {

struct SeriesDefinition {
    std::string display_name;
    std::function<ErrorOr<Limits>(NodePlotFile*, EvaluatedNodeGraph*, const GenericSeries& series)> get_limits;
    std::function<ErrorOr<void>(NodePlotFile*, EvaluatedNodeGraph*, const GenericSeries& series, Figure& figure, const FigureBounds& bounds)> evalulate;
};

struct NodeRegistry {
    static std::map<NodeTypeId, Node> node_map;

    static std::map<SeriesTypeId, SeriesDefinition> series_map;

    static void init();

    static void register_node(NodeTypeId node_id, Node node);
    static void register_series(SeriesTypeId series_id, Node::InputGenerator input_generator, SeriesDefinition series);
};

} // namespace NodePlot

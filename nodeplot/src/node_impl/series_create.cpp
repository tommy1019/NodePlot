#include "nodeplot.h"

using namespace NodePlot;

void register_series_create() {
    NodeRegistry::register_node(
        "scatter_series_create",
        Node{
            .type_id = "scatter_series_create",
            .display_name = "Scatter Series Create",
            .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                return {
                    {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::COLUMN}}},
                    {"y", Node::Input{.id = "y", .display_name = "Y", .valid_data_types = {DataType::COLUMN}}},
                    {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR}, .default_value = Color{.r = 1, .g = 0, .b = 0, .a = 1}}},
                    {"point_size", Node::Input{.id = "point_size", .display_name = "Point Size", .valid_data_types = {DataType::NUMBER}, .default_value = 3.0}},
                };
            },
            .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                return {
                    {"series", Node::Output{.id = "series", .display_name = "Series", .valid_data_types = {DataType::SERIES}}},
                };
            },
            .evalulate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                Column x = TRY(eng->get_input_value<Column>(npf, node_id, "x"));
                Column y = TRY(eng->get_input_value<Column>(npf, node_id, "y"));

                Color color = TRY(eng->get_input_value<Color>(npf, node_id, "color"));
                double point_size = TRY(eng->get_input_value<double>(npf, node_id, "point_size"));

                cache.computed_outputs["series"] = Series{ScatterSeries{
                    .x = x,
                    .y = y,
                    .color = color,
                    .point_size = point_size,
                }};

                return {};
            },
        });

    NodeRegistry::register_node(
        "line_series_create",
        Node{
            .type_id = "line_series_create",
            .display_name = "Line Series Create",
            .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                return {
                    {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::COLUMN}}},
                    {"y", Node::Input{.id = "y", .display_name = "Y", .valid_data_types = {DataType::COLUMN}}},
                    {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR}, .default_value = Color{.r = 1, .g = 0, .b = 0, .a = 1}}},
                    {"stroke_width", Node::Input{.id = "stroke_width", .display_name = "Stroke Width", .valid_data_types = {DataType::NUMBER}, .default_value = 3.0}},
                };
            },
            .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                return {
                    {"series", Node::Output{.id = "series", .display_name = "Series", .valid_data_types = {DataType::SERIES}}},
                };
            },
            .evalulate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                Column x = TRY(eng->get_input_value<Column>(npf, node_id, "x"));
                Column y = TRY(eng->get_input_value<Column>(npf, node_id, "y"));

                Color color = TRY(eng->get_input_value<Color>(npf, node_id, "color"));
                double stroke_width = TRY(eng->get_input_value<double>(npf, node_id, "stroke_width"));

                cache.computed_outputs["series"] = Series{LineSeries{
                    .x = x,
                    .y = y,
                    .color = color,
                    .stroke_width = stroke_width,
                }};

                return {};
            },
        });

    NodeRegistry::register_node(
        "ribbon_series_create",
        Node{
            .type_id = "ribbon_series_create",
            .display_name = "Ribbon Series Create",
            .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                return {
                    {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::COLUMN}}},
                    {"y_min", Node::Input{.id = "y_min", .display_name = "Y Min", .valid_data_types = {DataType::COLUMN}}},
                    {"y_max", Node::Input{.id = "y_max", .display_name = "Y Max", .valid_data_types = {DataType::COLUMN}}},
                    {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR}, .default_value = Color{.r = 1, .g = 0, .b = 0, .a = 0.2}}},
                };
            },
            .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                return {
                    {"series", Node::Output{.id = "series", .display_name = "Series", .valid_data_types = {DataType::SERIES}}},
                };
            },
            .evalulate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                Column x = TRY(eng->get_input_value<Column>(npf, node_id, "x"));
                Column y_min = TRY(eng->get_input_value<Column>(npf, node_id, "y_min"));
                Column y_max = TRY(eng->get_input_value<Column>(npf, node_id, "y_max"));

                Color color = TRY(eng->get_input_value<Color>(npf, node_id, "color"));

                cache.computed_outputs["series"] = Series{RibbonSeries{
                    .x = x,
                    .y_min = y_min,
                    .y_max = y_max,
                    .color = color,
                }};

                return {};
            },
        });
}

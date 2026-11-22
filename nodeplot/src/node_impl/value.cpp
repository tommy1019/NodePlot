#include "nodeplot.h"
#include "types.h"
#include <cmath>
#include <variant>
#include <vector>

using namespace NodePlot;

void register_value() {

    NodeRegistry::register_node("sequence",
                                Node{
                                    .type_id = "sequence",
                                    .display_name = "Sequence",
                                    .toolbar_path = {"Value", "Sequence"},
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"start", Node::Input{.id = "start", .display_name = "Start", .valid_data_types = {DataType::NUMBER}, .default_value = 0}},
                                            {"step", Node::Input{.id = "step", .display_name = "Step", .valid_data_types = {DataType::NUMBER}, .default_value = 1}},
                                            {"count", Node::Input{.id = "count", .display_name = "Count", .valid_data_types = {DataType::NUMBER}, .default_value = 10}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"values", Node::Output{.id = "values", .display_name = "Values", .valid_data_types = {DataType::NUMBER}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                                        auto start = TRY(eng->get_input_value<double>(npf, node_id, "start", true));
                                        auto step = TRY(eng->get_input_value<double>(npf, node_id, "step", true));
                                        auto count = TRY(eng->get_input_value<double>(npf, node_id, "count", true));

                                        std::vector<double> res;
                                        for (size_t i = 0; i < count; i++) {
                                            res.push_back(start + step * i);
                                        }

                                        output["values"] = res;

                                        return {};
                                    },
                                });

    NodeRegistry::register_node("range",
                                Node{
                                    .type_id = "range",
                                    .display_name = "Range",
                                    .toolbar_path = {"Value", "Range"},
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"start", Node::Input{.id = "start", .display_name = "Start", .valid_data_types = {DataType::NUMBER}, .default_value = 0}},
                                            {"end", Node::Input{.id = "end", .display_name = "End", .valid_data_types = {DataType::NUMBER}, .default_value = 1}},
                                            {"count", Node::Input{.id = "count", .display_name = "Count", .valid_data_types = {DataType::NUMBER}, .default_value = 10}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"values", Node::Output{.id = "values", .display_name = "Values", .valid_data_types = {DataType::NUMBER}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                                        auto start = TRY(eng->get_input_value<double>(npf, node_id, "start", true));
                                        auto end = TRY(eng->get_input_value<double>(npf, node_id, "end", true));
                                        auto count = TRY(eng->get_input_value<double>(npf, node_id, "count", true));

                                        double range = end - start;
                                        double step = range / (count - 1);

                                        std::vector<double> res;

                                        for (size_t i = 0; i < count; i++) {
                                            res.push_back(start + step * i);
                                        }

                                        output["values"] = res;

                                        return {};
                                    },
                                });

    NodeRegistry::register_node("numeric_value",
                                Node{
                                    .type_id = "numeric_value",
                                    .display_name = "Numeric Value",
                                    .toolbar_path = {"Value", "Numeric"},
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"value", Node::Input{.id = "value", .display_name = "Value", .valid_data_types = {DataType::NUMBER}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"value", Node::Output{.id = "value", .display_name = "Value", .valid_data_types = {DataType::NUMBER}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                                        output["value"] = TRY(eng->get_input_value<double>(npf, node_id, "value", true));
                                        return {};
                                    },
                                });

    NodeRegistry::register_node("string_value",
                                Node{
                                    .type_id = "string_value",
                                    .display_name = "String Value",
                                    .toolbar_path = {"Value", "String"},
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"value", Node::Input{.id = "value", .display_name = "Value", .valid_data_types = {DataType::STRING}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"value", Node::Output{.id = "value", .display_name = "Value", .valid_data_types = {DataType::STRING}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                                        output["value"] = TRY(eng->get_input_value<std::string>(npf, node_id, "value", true));
                                        return {};
                                    },
                                });

    NodeRegistry::register_node("color_value",
                                Node{
                                    .type_id = "color_value",
                                    .display_name = "Color Value",
                                    .toolbar_path = {"Value", "Color", "Value"},
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"value", Node::Input{.id = "value", .display_name = "Value", .valid_data_types = {DataType::COLOR}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"value", Node::Output{.id = "value", .display_name = "Value", .valid_data_types = {DataType::COLOR}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                                        output["value"] = TRY(eng->get_input_value<Color>(npf, node_id, "value", true));
                                        return {};
                                    },
                                });

    NodeRegistry::register_node("color_compose_rgb",
                                Node{
                                    .type_id = "color_compose_rgb",
                                    .display_name = "Color Compose RGB",
                                    .toolbar_path = {"Value", "Color", "Compose RGB"},
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"red", Node::Input{.id = "red", .display_name = "Red", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}, .default_value = 1.0}},
                                            {"green", Node::Input{.id = "green", .display_name = "Green", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}, .default_value = 0.0}},
                                            {"blue", Node::Input{.id = "blue", .display_name = "Blue", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}, .default_value = 0.0}},
                                            {"alpha", Node::Input{.id = "alpha", .display_name = "Alpha", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}, .default_value = 1.0}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"color", Node::Output{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR, DataType::COLOR_COLUMN}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                                        output["color"] = TRY(Utils::vectorized(
                                            [](double r, double g, double b, double a) -> Color {
                                                return Color{
                                                    .r = (float)r,
                                                    .g = (float)g,
                                                    .b = (float)b,
                                                    .a = (float)a,
                                                };
                                            },
                                            std::make_pair(std::string("red"), TRY(eng->get_input_value_variant<double, std::vector<double>>(npf, node_id, "red", true))),
                                            std::make_pair(std::string("green"), TRY(eng->get_input_value_variant<double, std::vector<double>>(npf, node_id, "green", true))),
                                            std::make_pair(std::string("blue"), TRY(eng->get_input_value_variant<double, std::vector<double>>(npf, node_id, "blue", true))),
                                            std::make_pair(std::string("alpha"), TRY(eng->get_input_value_variant<double, std::vector<double>>(npf, node_id, "alpha", true)))));
                                        return {};
                                    },
                                });

    NodeRegistry::register_node(
        "color_compose_hsv",
        Node{
            .type_id = "color_compose_hsv",
            .display_name = "Color Compose HSV",
            .toolbar_path = {"Value", "Color", "Compose HSV"},
            .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                return {
                    {"hue", Node::Input{.id = "hue", .display_name = "Hue", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}, .default_value = 180.0}},
                    {"saturation", Node::Input{.id = "saturation", .display_name = "Saturation", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}, .default_value = 1.0}},
                    {"value", Node::Input{.id = "value", .display_name = "Value", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}, .default_value = 1.0}},
                    {"alpha", Node::Input{.id = "alpha", .display_name = "Alpha", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}, .default_value = 1.0}},
                };
            },
            .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                return {
                    {"color", Node::Output{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR, DataType::COLOR_COLUMN}}},
                };
            },
            .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                output["color"] = TRY(Utils::vectorized(
                    [](double h, double s, double v, double a) -> Color {
                        if (std::isnan(h) || std::isinf(h))
                            h = 0.0;
                        h = std::fmod(h, 360.0);
                        while (h < 0)
                            h += 360;
                        h = (int)h % 360 + (h - std::floor(h));
                        h /= 360.0f;

                        int i = floor(h * 6);
                        float f = h * 6 - i;
                        float p = v * (1 - s);
                        float q = v * (1 - f * s);
                        float t = v * (1 - (1 - f) * s);

                        float r, g, b;
                        switch (i % 6) {
                        case 0:
                            r = v, g = t, b = p;
                            break;
                        case 1:
                            r = q, g = v, b = p;
                            break;
                        case 2:
                            r = p, g = v, b = t;
                            break;
                        case 3:
                            r = p, g = q, b = v;
                            break;
                        case 4:
                            r = t, g = p, b = v;
                            break;
                        case 5:
                            r = v, g = p, b = q;
                            break;
                        }

                        return Color{
                            .r = (float)r,
                            .g = (float)g,
                            .b = (float)b,
                            .a = (float)a,
                        };
                    },
                    std::make_pair(std::string("hue"), TRY(eng->get_input_value_variant<double, std::vector<double>>(npf, node_id, "hue", true))),
                    std::make_pair(std::string("saturation"), TRY(eng->get_input_value_variant<double, std::vector<double>>(npf, node_id, "saturation", true))),
                    std::make_pair(std::string("value"), TRY(eng->get_input_value_variant<double, std::vector<double>>(npf, node_id, "value", true))),
                    std::make_pair(std::string("alpha"), TRY(eng->get_input_value_variant<double, std::vector<double>>(npf, node_id, "alpha", true)))));
                return {};
            },
        });

    NodeRegistry::register_node("color_decompose",
                                Node{
                                    .type_id = "color_decompose",
                                    .display_name = "Color Decompose",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR, NodePlot::DataType::COLOR_COLUMN}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"red", Node::Output{.id = "red", .display_name = "Red", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}}},
                                            {"green", Node::Output{.id = "green", .display_name = "Green", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}}},
                                            {"blue", Node::Output{.id = "blue", .display_name = "Blue", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}}},
                                            {"alpha", Node::Output{.id = "alpha", .display_name = "Alpha", .valid_data_types = {DataType::NUMBER, DataType::NUMBER_COLUMN}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                                        auto color = TRY(eng->get_input_value_variant<Color, std::vector<Color>>(npf, node_id, "color", true));
                                        output["red"] = TRY(Utils::vectorized([](Color c) -> double { return c.r; }, std::make_pair(std::string("color"), color)));
                                        output["green"] = TRY(Utils::vectorized([](Color c) -> double { return c.g; }, std::make_pair(std::string("color"), color)));
                                        output["blue"] = TRY(Utils::vectorized([](Color c) -> double { return c.b; }, std::make_pair(std::string("color"), color)));
                                        output["alpha"] = TRY(Utils::vectorized([](Color c) -> double { return c.a; }, std::make_pair(std::string("color"), color)));
                                        return {};
                                    },
                                });
}

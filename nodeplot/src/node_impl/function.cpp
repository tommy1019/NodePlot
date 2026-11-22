#include "eval_node_graph.h"
#include "file.h"
#include "node_graph.h"
#include "node_registry.h"

using namespace NodePlot;

ErrorOr<DataType> parse_data_type(std::string s) {
    if (s == "number")
        return DataType::NUMBER;
    if (s == "number_column")
        return DataType::NUMBER_COLUMN;
    if (s == "integer")
        return DataType::INTEGER;
    if (s == "integer_column")
        return DataType::INTEGER_COLUMN;
    if (s == "string")
        return DataType::STRING;
    if (s == "string_column")
        return DataType::STRING_COLUMN;
    if (s == "bool")
        return DataType::BOOLEAN;
    if (s == "bool_column")
        return DataType::BOOLEAN_COLUMN;

    if (s == "table")
        return DataType::TABLE;

    if (s == "figure")
        return DataType::FIGURE;

    if (s == "series")
        return DataType::SERIES;

    if (s == "margins")
        return DataType::MARGINS;
    if (s == "color")
        return DataType::COLOR;
    if (s == "color_column")
        return DataType::COLOR_COLUMN;

    if (s == "plot_style")
        return DataType::PLOT_STYLE;

    return ERR("Unknown type: " + s);
}

void register_function() {
    NodeRegistry::register_node(
        "function",
        Node{
            .type_id = "function",
            .display_name = "Function",
            .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                std::vector<std::pair<InputId, Node::Input>> res;

                res.emplace_back("name", Node::Input{.id = "name", .display_name = "Function Name", .valid_data_types = {DataType::STRING}, .attributes = {InputAttribute::FunctionName{}}});

                [&]() -> ErrorOr<void> {
                    auto name_or_err = eng->get_input_value<std::string>(npf, node_id, "name", false);
                    if (!name_or_err.has_value())
                        return ERR("");
                    auto name = name_or_err.value();

                    NodeGraph& graph = TRY(Utils::try_find(npf->graphs, name, "")).get();

                    if (!graph.function_input_id.has_value())
                        return ERR("Function has invalid input id");
                    if (!graph.function_output_id.has_value())
                        return ERR("Function has invalid output id");

                    if (eng->cache[node_id].function_eval_eng.get() == nullptr) {
                        eng->cache[node_id].function_eval_eng.reset(new EvaluatedNodeGraph{.graph_id = name});
                    }
                    EvaluatedNodeGraph* tmp_eng = eng->cache[node_id].function_eval_eng.get();

                    int64_t input_count = std::clamp(tmp_eng->get_input_value<int64_t>(npf, graph.function_input_id.value(), "count", false).value_or(0), int64_t{0}, int64_t{255});

                    for (int64_t i = 0; i < input_count; i++) {
                        [&]() -> ErrorOr<void> {
                            std::string param_name = "input_" + std::to_string(i);
                            std::string name_field = param_name + "_name";
                            std::string type_field = param_name + "_type";

                            auto name_or_err = tmp_eng->get_input_value<std::string>(npf, graph.function_input_id.value(), name_field, false);
                            if (!name_or_err.has_value())
                                return ERR("");
                            auto name = name_or_err.value();

                            auto type_or_err = tmp_eng->get_input_value<std::string>(npf, graph.function_input_id.value(), type_field, false);
                            if (!type_or_err.has_value())
                                return ERR("");
                            DataType type = TRY(parse_data_type(type_or_err.value()));

                            res.emplace_back(param_name, Node::Input{.id = param_name, .display_name = "[" + std::to_string(i) + "] " + name, .valid_data_types = {type}});

                            return {};
                        }();
                    }

                    return {};
                }();

                return res;
            },
            .outputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<OutputId, Node::Output>> {
                std::vector<std::pair<OutputId, Node::Output>> res;

                auto _ = [&]() -> ErrorOr<void> {
                    auto name_or_err = eng->get_input_value<std::string>(npf, node_id, "name", false);
                    if (!name_or_err.has_value())
                        return ERR("");
                    auto name = name_or_err.value();

                    NodeGraph& graph = TRY(Utils::try_find(npf->graphs, name, "")).get();

                    if (!graph.function_input_id.has_value())
                        return ERR("Function has invalid input id");
                    if (!graph.function_output_id.has_value())
                        return ERR("Function has invalid output id");

                    if (eng->cache[node_id].function_eval_eng.get() == nullptr) {
                        eng->cache[node_id].function_eval_eng.reset(new EvaluatedNodeGraph{.graph_id = name});
                    }
                    EvaluatedNodeGraph* tmp_eng = eng->cache[node_id].function_eval_eng.get();

                    int64_t output_count = std::clamp(tmp_eng->get_input_value<int64_t>(npf, graph.function_output_id.value(), "count", false).value_or(0), int64_t{0}, int64_t{255});

                    for (int64_t i = 0; i < output_count; i++) {
                        auto __ = [&]() -> ErrorOr<void> {
                            std::string param_name = "output_" + std::to_string(i);
                            std::string name_field = param_name + "_name";
                            std::string type_field = param_name + "_type";

                            auto name_or_err = tmp_eng->get_input_value<std::string>(npf, graph.function_output_id.value(), name_field, false);
                            if (!name_or_err.has_value())
                                return ERR("Could not get name");
                            auto name = name_or_err.value();

                            auto type_or_err = tmp_eng->get_input_value<std::string>(npf, graph.function_output_id.value(), type_field, false);
                            if (!type_or_err.has_value())
                                return ERR("Could not get type");
                            DataType type = TRY(parse_data_type(type_or_err.value()));

                            res.emplace_back(param_name, Node::Output{.id = param_name, .display_name = "[" + std::to_string(i) + "] " + name, .valid_data_types = {type}});

                            return {};
                        }();
                    }

                    return {};
                }();

                return res;
            },
            .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                std::string function_name = TRY(eng->get_input_value<std::string>(npf, node_id, "name", true));

                auto graph = npf->graphs.find(function_name);
                if (graph == npf->graphs.end()) {
                    return std::unexpected(Node::EvaluationError{
                        .description = "Unknown function name",
                        .input_errors = {"name"},
                    });
                }

                if (!graph->second.function_input_id.has_value())
                    return std::unexpected(Node::EvaluationError{
                        .description = "Function has invalid input id",
                        .input_errors = {"name"},
                    });
                if (!graph->second.function_output_id.has_value())
                    return std::unexpected(Node::EvaluationError{
                        .description = "Function has invalid output id",
                        .input_errors = {"name"},
                    });

                eng->cache[node_id].function_eval_eng.reset(new EvaluatedNodeGraph{.graph_id = function_name});

                EvaluatedNodeGraph* func_eng = eng->cache[node_id].function_eval_eng.get();

                int64_t input_count = std::clamp(func_eng->get_input_value<int64_t>(npf, graph->second.function_input_id.value(), "count", true).value_or(0), int64_t{0}, int64_t{255});
                int64_t output_count = std::clamp(func_eng->get_input_value<int64_t>(npf, graph->second.function_output_id.value(), "count", true).value_or(0), int64_t{0}, int64_t{255});

                auto& input_cache = func_eng->cache[graph->second.function_input_id.value()];

                // Get inputs from this nodes inputs and put them in the output cache of the function's input node
                for (int64_t i = 0; i < input_count; i++) {

                    std::string param_name = "input_" + std::to_string(i);
                    std::string name_field = param_name + "_name";
                    std::string type_field = param_name + "_type";

                    auto unparsed_type = TRY(func_eng->get_input_value<std::string>(npf, graph->second.function_input_id.value(), type_field, false));
                    auto type_or_err = parse_data_type(unparsed_type);
                    if (!type_or_err.has_value()) {
                        return std::unexpected(Node::EvaluationError{
                            .description = "Function has invalid type: " + unparsed_type,
                            .input_errors = {param_name},
                        });
                    }

                    Data value;

                    if (type_or_err.value() != NodePlot::DataType::PLOT_STYLE) {
                        value = TRY(eng->get_input_data(npf, node_id, param_name, true));
                    } else {
                        value = TRY(eng->get_input_style(npf, node_id, param_name));
                    }

                    input_cache.computed_outputs[param_name] = value;
                }

                // Get outputs from the function's output node and put them in this nodes cache
                for (int64_t i = 0; i < output_count; i++) {
                    std::string param_name = "output_" + std::to_string(i);
                    auto value = TRY(func_eng->get_input_data(npf, graph->second.function_output_id.value(), param_name, true));
                    output[param_name] = value;
                }

                return {};
            },
        });

    NodeRegistry::register_node(
        "function_input",
        Node{
            .type_id = "function_input",
            .display_name = "Function Input",
            .toolbar_path = {"Debug", "Function", "Input"},
            .deletable = false,
            .cloneable = false,
            .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                std::vector<std::pair<InputId, Node::Input>> res;

                res.emplace_back("count", Node::Input{.id = "count", .display_name = "Count", .valid_data_types = {DataType::INTEGER}});

                int64_t count = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "count", false).value_or(0), int64_t{0}, int64_t{255});

                for (int64_t i = 0; i < count; i++) {
                    std::string param_name = "input_" + std::to_string(i);
                    std::string name_field = param_name + "_name";
                    std::string type_field = param_name + "_type";

                    res.emplace_back(name_field, Node::Input{.id = name_field, .display_name = std::to_string(i) + " Name", .valid_data_types = {DataType::STRING}});
                    res.emplace_back(type_field, Node::Input{.id = type_field, .display_name = std::to_string(i) + " Type", .valid_data_types = {DataType::STRING}});
                }

                return res;
            },
            .outputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<OutputId, Node::Output>> {
                std::vector<std::pair<OutputId, Node::Output>> res;

                int64_t count = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "count", false).value_or(0), int64_t{0}, int64_t{255});

                for (int64_t i = 0; i < count; i++) {
                    std::string param_name = "input_" + std::to_string(i);
                    std::string name_field = param_name + "_name";
                    std::string type_field = param_name + "_type";

                    auto name = eng->get_input_value<std::string>(npf, node_id, name_field, false);
                    auto type = eng->get_input_value<std::string>(npf, node_id, type_field, false).and_then([](std::string s) -> std::expected<DataType, Node::EvaluationError> {
                        auto parse_or_error = parse_data_type(s);
                        if (!parse_or_error.has_value())
                            return std::unexpected(Node::EvaluationError{});
                        return parse_or_error.value();
                    });
                    if (type.has_value() && name.has_value()) {
                        res.emplace_back(param_name, Node::Output{.id = param_name, .display_name = "[" + std::to_string(i) + "] " + name.value() + " Value", .valid_data_types = {type.value()}});
                    }
                }

                return res;
            },
            .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                return std::unexpected(Node::EvaluationError{
                    .description = "Can't evaluate function input node",
                    .input_errors = {},
                });
            },
        });

    NodeRegistry::register_node("function_output",
                                Node{
                                    .type_id = "function_output",
                                    .display_name = "Function Output",
                                    .toolbar_path = {"Debug", "Function", "Output"},
                                    .deletable = false,
                                    .cloneable = false,
                                    .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                                        std::vector<std::pair<InputId, Node::Input>> res;

                                        res.emplace_back("count", Node::Input{.id = "count", .display_name = "Count", .valid_data_types = {DataType::INTEGER}});

                                        int64_t count = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "count", false).value_or(0), int64_t{0}, int64_t{255});

                                        for (int64_t i = 0; i < count; i++) {
                                            std::string param_name = "output_" + std::to_string(i);
                                            std::string name_field = param_name + "_name";
                                            std::string type_field = param_name + "_type";

                                            res.emplace_back(name_field, Node::Input{.id = name_field, .display_name = std::to_string(i) + " Name", .valid_data_types = {DataType::STRING}});
                                            res.emplace_back(type_field, Node::Input{.id = type_field, .display_name = std::to_string(i) + " Type", .valid_data_types = {DataType::STRING}});
                                            res.emplace_back(param_name, Node::Input{.id = param_name, .display_name = std::to_string(i) + " Value", .valid_data_types = {DataType::STRING}});
                                        }

                                        return res;
                                    },
                                    .outputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        std::vector<std::pair<OutputId, Node::Output>> res;
                                        return res;
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                                        return std::unexpected(Node::EvaluationError{
                                            .description = "Can't evaluate function output node",
                                            .input_errors = {},
                                        });
                                    },
                                });
}

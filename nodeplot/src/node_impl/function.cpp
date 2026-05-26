#include "error.h"
#include "node.h"
#include "nodeplot.h"
#include "types.h"
#include "utils.h"
#include <string>

using namespace NodePlot;

ErrorOr<DataType> parse_data_type(std::string s) {
    if (s == "number")
        return DataType::NUMBER;
    if (s == "integer")
        return DataType::INTEGER;
    if (s == "string")
        return DataType::STRING;
    if (s == "bool")
        return DataType::BOOLEAN;

    if (s == "table")
        return DataType::TABLE;

    if (s == "column")
        return DataType::COLUMN;

    if (s == "series")
        return DataType::SERIES;

    if (s == "margines")
        return DataType::MARGINES;
    if (s == "color")
        return DataType::COLOR;

    if (s == "plot_style")
        return DataType::PLOT_STYLE;

    return ERR("Unknown type: " + s);
}

void register_function() {
    NodeRegistry::register_node("function",
                                Node{
                                    .type_id = "function",
                                    .display_name = "Function",
                                    .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                                        std::vector<std::pair<InputId, Node::Input>> res;

                                        res.emplace_back(
                                            "name", Node::Input{.id = "name", .display_name = "Function Name", .valid_data_types = {DataType::STRING}, .attributes = {InputAttribute::FunctionName{}}});

                                        [&]() -> ErrorOr<void> {
                                            std::string name = TRY(eng->get_input_value<std::string>(npf, node_id, "name"));

                                            NodeGraph& graph = TRY(Utils::try_find(npf->graphs, name, "")).get();

                                            if (!graph.function_input_id.has_value())
                                                return ERR("Function has invalid input id");
                                            if (!graph.function_output_id.has_value())
                                                return ERR("Function has invalid output id");

                                            EvaluatedNodeGraph tmp_eng{.graph_id = name};

                                            int64_t input_count = std::clamp(tmp_eng.get_input_value<int64_t>(npf, graph.function_input_id.value(), "count").value_or(0), int64_t{0}, int64_t{255});

                                            for (int64_t i = 0; i < input_count; i++) {
                                                [&]() -> ErrorOr<void> {
                                                    std::string param_name = "input_" + std::to_string(i);
                                                    std::string name_field = param_name + "_name";
                                                    std::string type_field = param_name + "_type";

                                                    std::string name = TRY(tmp_eng.get_input_value<std::string>(npf, graph.function_input_id.value(), name_field));
                                                    DataType type = TRY(parse_data_type(TRY(tmp_eng.get_input_value<std::string>(npf, graph.function_input_id.value(), type_field))));

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

                                        [&]() -> ErrorOr<void> {
                                            std::string name = TRY(eng->get_input_value<std::string>(npf, node_id, "name"));

                                            NodeGraph& graph = TRY(Utils::try_find(npf->graphs, name, "")).get();

                                            if (!graph.function_input_id.has_value())
                                                return ERR("Function has invalid input id");
                                            if (!graph.function_output_id.has_value())
                                                return ERR("Function has invalid output id");

                                            EvaluatedNodeGraph tmp_eng{.graph_id = name};

                                            int64_t output_count = std::clamp(tmp_eng.get_input_value<int64_t>(npf, graph.function_output_id.value(), "count").value_or(0), int64_t{0}, int64_t{255});

                                            for (int64_t i = 0; i < output_count; i++) {
                                                [&]() -> ErrorOr<void> {
                                                    std::string param_name = "output_" + std::to_string(i);
                                                    std::string name_field = param_name + "_name";
                                                    std::string type_field = param_name + "_type";

                                                    std::string name = TRY(tmp_eng.get_input_value<std::string>(npf, graph.function_output_id.value(), name_field));
                                                    DataType type = TRY(parse_data_type(TRY(tmp_eng.get_input_value<std::string>(npf, graph.function_output_id.value(), type_field))));

                                                    res.emplace_back(param_name, Node::Output{.id = param_name, .display_name = "[" + std::to_string(i) + "] " + name, .valid_data_types = {type}});

                                                    return {};
                                                }();
                                            }

                                            return {};
                                        }();

                                        return res;
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        std::string function_name = TRY(eng->get_input_value<std::string>(npf, node_id, "name"));
                                        NodeGraph& graph = TRY(Utils::try_find(npf->graphs, function_name, "Unknown function name")).get();

                                        if (!graph.function_input_id.has_value())
                                            return ERR("Function has invalid input id");
                                        if (!graph.function_output_id.has_value())
                                            return ERR("Function has invalid output id");

                                        EvaluatedNodeGraph func_eng{.graph_id = function_name};

                                        int64_t input_count = std::clamp(func_eng.get_input_value<int64_t>(npf, graph.function_input_id.value(), "count").value_or(0), int64_t{0}, int64_t{255});
                                        int64_t output_count = std::clamp(func_eng.get_input_value<int64_t>(npf, graph.function_output_id.value(), "count").value_or(0), int64_t{0}, int64_t{255});

                                        auto& input_cache = func_eng.cache[graph.function_input_id.value()];

                                        // Get inputs from this nodes inputs and put them in the output cache of the function's input node
                                        for (int64_t i = 0; i < input_count; i++) {
                                            std::string param_name = "input_" + std::to_string(i);
                                            auto value = TRY(eng->get_input_data(npf, node_id, param_name));
                                            input_cache.computed_outputs[param_name] = value;
                                        }

                                        // Get outputs from the function's output node and put them in this nodes cache
                                        for (int64_t i = 0; i < output_count; i++) {
                                            std::string param_name = "output_" + std::to_string(i);
                                            auto value = TRY(func_eng.get_input_data(npf, graph.function_output_id.value(), param_name));
                                            cache.computed_outputs[param_name] = value;
                                        }

                                        return {};
                                    },
                                });

    NodeRegistry::register_node(
        "function_input",
        Node{
            .type_id = "function_input",
            .display_name = "Function Input",
            .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                std::vector<std::pair<InputId, Node::Input>> res;

                res.emplace_back("count", Node::Input{.id = "count", .display_name = "Count", .valid_data_types = {DataType::INTEGER}});

                int64_t count = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "count").value_or(0), int64_t{0}, int64_t{255});

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

                int64_t count = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "count").value_or(0), int64_t{0}, int64_t{255});

                for (int64_t i = 0; i < count; i++) {
                    std::string param_name = "input_" + std::to_string(i);
                    std::string name_field = param_name + "_name";
                    std::string type_field = param_name + "_type";

                    ErrorOr<std::string> name = eng->get_input_value<std::string>(npf, node_id, name_field);
                    ErrorOr<DataType> type = eng->get_input_value<std::string>(npf, node_id, type_field).and_then([](std::string s) { return parse_data_type(s); });
                    if (type.has_value() && name.has_value()) {
                        res.emplace_back(param_name, Node::Output{.id = param_name, .display_name = "[" + std::to_string(i) + "] " + name.value() + " Value", .valid_data_types = {type.value()}});
                    }
                }

                return res;
            },
            .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> { return ERR("Missing input paramater"); },
        });

    NodeRegistry::register_node("function_output",
                                Node{
                                    .type_id = "function_output",
                                    .display_name = "Function Output",
                                    .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                                        std::vector<std::pair<InputId, Node::Input>> res;

                                        res.emplace_back("count", Node::Input{.id = "count", .display_name = "Count", .valid_data_types = {DataType::INTEGER}});

                                        int64_t count = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "count").value_or(0), int64_t{0}, int64_t{255});

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
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        return ERR("Output node must output from function call");
                                    },
                                });
}

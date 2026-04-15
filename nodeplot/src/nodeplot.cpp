#include "nodeplot.h"
#include "nlohmann/json_fwd.hpp"

#include <filesystem>

#include <sys/mman.h>

extern void register_binary_operation();
extern void register_column_select();
extern void register_create_plot();
extern void register_csv_import();
extern void register_filter_table();
extern void register_sampled_property_extract();
extern void register_series_create();
extern void register_value();

void register_output();

namespace NodePlot {

std::map<NodeTypeId, Node> NodeRegistry::node_map = {};

void NodeRegistry::init() {
    register_binary_operation();
    register_column_select();
    register_create_plot();
    register_csv_import();
    register_filter_table();
    register_sampled_property_extract();
    register_series_create();
    register_value();
    register_output();
}

void NodeRegistry::NodeRegistry::register_node(NodeTypeId node_id, Node node) { NodeRegistry::node_map.insert({node_id, node}); }

ErrorOr<Data> EvaluatedNodeGraph::get_output_data(NodePlotFile* npf, NodeId node_id, OutputId output_id) {
    NodeOutputCache& output_cache = cache[node_id];

    auto data = output_cache.computed_outputs.find(output_id);
    if (data != output_cache.computed_outputs.end()) {
        return data->second;
    }

    auto& ng = TRY(node_graph(npf)).get();
    NodeGraph::NodeStorage& node_input_storage = TRY(Utils::try_find(ng.nodes, node_id, "Invalid NodeID")).get();
    Node& node = TRY(Utils::try_find(NodeRegistry::node_map, node_input_storage.type_id, "Invalid NodeTypeId")).get();

    auto eval_err = node.evalulate(npf, this, node_id, output_cache);
    if (!eval_err.has_value()) {
        output_cache.error = eval_err.error();
    }

    return TRY(Utils::try_find(output_cache.computed_outputs, output_id, "OutputId not found")).get();
}

ErrorOr<Data> EvaluatedNodeGraph::get_input_data(NodePlotFile* npf, NodeId node_id, InputId input_id) {
    auto& ng = TRY(node_graph(npf)).get();
    NodeGraph::NodeStorage& node_input_storage = TRY(Utils::try_find(ng.nodes, node_id, "Invalid NodeID")).get();
    auto& input_storage = TRY(Utils::try_find(node_input_storage.input_storage, input_id, "Invalid InputID: " + input_id)).get();

    return std::visit(overloaded{
                          [&](Data data) -> ErrorOr<Data> { return data; },
                          [&](NodeGraph::InputPin input_pin) -> ErrorOr<Data> { return this->get_output_data(npf, input_pin.node_id, input_pin.output_id); },
                      },
                      input_storage);
}

MappedFile::~MappedFile() {
    if (munmap(data, file_size) == -1) {
        REQUIRE_NOT_REACHED("Failed to unmap CSV file");
    }
}

ErrorOr<NodePlotFile> NodePlotFile::create(std::filesystem::path path) {
    NodePlotFile res;
    res.path = path;
    res.graphs = {{"main", NodeGraph{}}};
    return res;
}

ErrorOr<NodePlotFile> NodePlotFile::from_json(nlohmann::json json, std::filesystem::path path) {
    NodePlotFile res;
    res.path = path;

    auto graphs = json.find("graphs");
    if (graphs == json.end())
        return ERR("Missing graphs");

    for (auto it = graphs.value().begin(); it != graphs.value().end(); it++) {
        res.graphs[it.key()] = TRY(NodeGraph::from_json(it.value()));
    }

    return res;
}

ErrorOr<nlohmann::json> NodePlotFile::to_json() {
    nlohmann::json graphs;

    for (auto [graph_id, graph] : this->graphs) {
        graphs[graph_id] = TRY(graph.to_json());
    }

    nlohmann::json res;
    res["graphs"] = graphs;

    return res;
};

ErrorOr<NodeGraph> NodeGraph::from_json(nlohmann::json json) {
    auto get = [](nlohmann::json json, const char* key, std::string err) -> ErrorOr<nlohmann::json> {
        auto res = json.find(key);
        if (res == json.end())
            return ERR(err);
        return res.value();
    };

    auto get_typed = [&]<typename T>(std::type_identity<T>, nlohmann::json json, const char* key, std::string err) -> ErrorOr<T> {
        auto j = TRY(get(json, key, err));
        try {
            return j.get<T>();
        } catch (nlohmann::json::type_error) {
            return ERR(err);
        }
    };

    NodeGraph res;

    nlohmann::json nodes_json = TRY(get(json, "nodes", "Missing nodes data"));
    for (auto nodes_it = nodes_json.begin(); nodes_it != nodes_json.end(); nodes_it++) {
        NodeId node_id = TRY([&]() -> ErrorOr<NodeId> {
            try {
                return static_cast<NodeId>(std::stoll(nodes_it.key()));
            } catch (const std::out_of_range& e) {
                return ERR("Invalid NodeId: " + nodes_it.key());
            } catch (const std::invalid_argument& e) {
                return ERR("Invalid NodeId: " + nodes_it.key());
            }
        }());
        res.next_free_node_id = std::max(res.next_free_node_id, node_id + 1);

        NodeStorage node_storage;
        node_storage.type_id = TRY(get_typed(std::type_identity<NodeTypeId>{}, nodes_it.value(), "type_id", "Missing NodeTypeId for node " + std::to_string(node_id)));

        auto pos_json = TRY(get_typed(std::type_identity<nlohmann::json>{}, nodes_it.value(), "pos", "Missing node position for node " + std::to_string(node_id)));
        if (!pos_json.is_array() && pos_json.size() == 2)
            return ERR("Malformed node position data for node " + std::to_string(node_id));
        try {
            node_storage.pos.x = pos_json[0].get<double>();
            node_storage.pos.y = pos_json[1].get<double>();
        } catch (nlohmann::json::type_error) {
            return ERR("Malformed node position data for node " + std::to_string(node_id));
        }

        if (!NodeRegistry::node_map.contains(node_storage.type_id))
            return ERR("Invalid NodeTypeId for node " + std::to_string(node_id));

        auto inputs_json = TRY(get(nodes_it.value(), "inputs", "Missing Node Inputs for node " + std::to_string(node_id)));
        for (auto inputs_it = inputs_json.begin(); inputs_it != inputs_json.end(); inputs_it++) {
            nlohmann::json input_json = inputs_it.value();

            auto err_str = [&]() { return "Node [" + std::to_string(node_id) + "] Input '" + inputs_it.key() + "': "; };

            auto input_val = TRY([&]() -> ErrorOr<std::variant<Data, InputPin>> {
                std::string input_type = TRY(get_typed(std::type_identity<std::string>{}, input_json, "type", err_str() + "Missing input type"));
                if (input_type == "pin") {
                    InputPin pin;
                    pin.node_id = TRY(get_typed(std::type_identity<NodeId>{}, input_json, "node", err_str() + "Missing input pin NodeId"));
                    pin.output_id = TRY(get_typed(std::type_identity<OutputId>{}, input_json, "output", err_str() + "Missing input pin OutputId"));
                    return pin;
                } else if (input_type == "data") {
                    std::string data_type = TRY(get_typed(std::type_identity<std::string>{}, input_json, "data_type", err_str() + "Missing input data type"));
                    if (data_type == "number")
                        return Data(TRY(get_typed(std::type_identity<double>{}, input_json, "value", err_str() + "Missing input data value double")));
                    else if (data_type == "string")
                        return Data(TRY(get_typed(std::type_identity<std::string>{}, input_json, "value", err_str() + "Missing input data value string")));
                    else if (data_type == "boolean")
                        return Data(TRY(get_typed(std::type_identity<bool>{}, input_json, "value", err_str() + "Missing input data value boolean")));
                    else if (data_type == "integer")
                        return Data(TRY(get_typed(std::type_identity<int64_t>{}, input_json, "value", err_str() + "Missing input data value integer")));
                    else if (data_type == "color") {
                        std::vector<float> color_arr = TRY(get_typed(std::type_identity<std::vector<float>>{}, input_json, "value", err_str() + "Missing input data value color"));
                        if (color_arr.size() != 4)
                            return ERR("Invalid color data");
                        return Data(Color{color_arr[0], color_arr[1], color_arr[2], color_arr[3]});
                    } else if (data_type == "margin") {
                        std::vector<float> margin_arr = TRY(get_typed(std::type_identity<std::vector<float>>{}, input_json, "value", err_str() + "Missing input data value color"));
                        if (margin_arr.size() != 4)
                            return ERR(err_str() + "Invalid margin data");
                        return Data(NodePlot::Margins{margin_arr[0], margin_arr[1], margin_arr[2], margin_arr[3]});
                    } else
                        return ERR(err_str() + "Invalid data type for input");
                } else {
                    return ERR(err_str() + "Input type is invalid");
                }
            }());

            node_storage.input_storage[inputs_it.key()] = input_val;
        }

        res.nodes[node_id] = node_storage;
    }

    return res;
}

ErrorOr<nlohmann::json> NodeGraph::to_json() {
    nlohmann::json nodes_json;

    for (auto [node_id, node_storage] : nodes) {
        nlohmann::json node_inputs;
        for (auto [input_id, data_or_pin] : node_storage.input_storage) {
            node_inputs[input_id] = TRY(std::visit(overloaded{
                                                       [](Data data) -> ErrorOr<nlohmann::json> {
                                                           nlohmann::json res;
                                                           res["type"] = "data";

                                                           TRY(std::visit(overloaded{[&](double v) -> ErrorOr<void> {
                                                                                         res["data_type"] = "number";
                                                                                         res["value"] = v;
                                                                                         return {};
                                                                                     },
                                                                                     [&](std::string v) -> ErrorOr<void> {
                                                                                         res["data_type"] = "string";
                                                                                         res["value"] = v;
                                                                                         return {};
                                                                                     },
                                                                                     [&](bool v) -> ErrorOr<void> {
                                                                                         res["data_type"] = "boolean";
                                                                                         res["value"] = v;
                                                                                         return {};
                                                                                     },
                                                                                     [&](int64_t v) -> ErrorOr<void> {
                                                                                         res["data_type"] = "integer";
                                                                                         res["value"] = v;
                                                                                         return {};
                                                                                     },
                                                                                     [&](Color v) -> ErrorOr<void> {
                                                                                         res["data_type"] = "color";
                                                                                         res["value"] = std::vector<float>{v.r, v.g, v.b, v.a};
                                                                                         return {};
                                                                                     },
                                                                                     [&](Margins v) -> ErrorOr<void> {
                                                                                         res["data_type"] = "margin";
                                                                                         res["value"] = std::vector<float>{v.left, v.right, v.top, v.bottom};
                                                                                         return {};
                                                                                     },
                                                                                     [&](auto) -> ErrorOr<void> { return ERR("Cannot encode input value"); }},
                                                                          data));

                                                           return res;
                                                       },
                                                       [](InputPin pin) -> ErrorOr<nlohmann::json> {
                                                           nlohmann::json res;
                                                           res["type"] = "pin";
                                                           res["node"] = pin.node_id;
                                                           res["output"] = pin.output_id;
                                                           return res;
                                                       },
                                                   },
                                                   data_or_pin));
        }

        nlohmann::json node_json;
        node_json["inputs"] = node_inputs;
        node_json["type_id"] = node_storage.type_id;
        node_json["pos"] = {node_storage.pos.x, node_storage.pos.y};
        nodes_json[std::to_string(node_id)] = node_json;
    }

    nlohmann::json res;
    res["nodes"] = nodes_json;

    return res;
}

ErrorOr<NodeId> NodeGraph::create_node(NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeTypeId type, float x, float y) {
    auto node = TRY(Utils::try_find(NodeRegistry::node_map, type, "Invalid node type id")).get();

    NodeId id = next_free_node_id++;

    auto& storage = nodes[id];
    storage = {
        .type_id = type,
        .pos = {.x = x, .y = y},
    };

    auto inputs = node.inputs(npf, eng, id);
    if (inputs.has_value()) {
        for (auto& i : inputs.value()) {
            if (i.second.default_value.has_value())
                storage.input_storage[i.first] = i.second.default_value.value();
        }
    }

    return id;
}

} // namespace NodePlot

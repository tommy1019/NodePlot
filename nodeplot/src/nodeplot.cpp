#include "nodeplot.h"
#include "nlohmann/json_fwd.hpp"

#include <filesystem>
#include <limits>
#include <sys/mman.h>

#include <charconv>
#include <fstream>
#include <variant>

namespace NodePlot {

std::map<NodeTypeId, Node> NodePlot::node_map = {};

void NodePlot::init() {
    NodePlot::register_node("csv_import",
                            Node{
                                .type_id = "csv_import",
                                .display_name = "CSV Import",
                                .inputs = [](EvaluatedNodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::map<InputId, Node::Input> {
                                    return {
                                        {"filename", Node::Input{.id = "filename", .display_name = "Filename", .data_type = DataType::FILE_PATH}},
                                        {"has_headers", Node::Input{.id = "has_headers", .display_name = "Has Headers", .data_type = DataType::BOOLEAN}},
                                    };
                                },
                                .outputs = [](EvaluatedNodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::map<OutputId, Node::Output> {
                                    return {
                                        {"table", Node::Output{.id = "table", .display_name = "Table", .data_type = DataType::TABLE}},
                                    };
                                },
                                .evalulate = [](EvaluatedNodePlotFile* enpf, EvaluatedNodeGraph* eng, NodeId node_id, NodeOutputCache cache) -> ErrorOr<void> {
                                    std::filesystem::path filename = TRY(eng->get_input_value<std::filesystem::path>(enpf, node_id, "filename"));
                                    bool has_headers = TRY(eng->get_input_value<bool>(enpf, node_id, "has_headers"));

                                    filename = enpf->node_plot_file.path.parent_path() / filename;

                                    std::print("Loading: {}\n", filename.string());

                                    if (!std::filesystem::exists(filename)) {
                                        return ERR("Input CSV file does not exist");
                                    }

                                    if (std::filesystem::is_directory(filename)) {
                                        return ERR("Input CSV file is a directory");
                                    }

                                    auto file_size = std::filesystem::file_size(filename);

                                    return {};
                                },
                            });
}

void NodePlot::NodePlot::register_node(NodeTypeId node_id, Node node) { NodePlot::node_map.insert({node_id, node}); }

ErrorOr<Data> EvaluatedNodeGraph::get_output_data(EvaluatedNodePlotFile* enpf, NodeId node_id, OutputId output_id) {
    NodeOutputCache& output_cache = cache[node_id];

    auto data = output_cache.outputs.find(output_id);
    if (data != output_cache.outputs.end()) {
        return data->second;
    }

    NodeGraph::InputStorage& node_input_storage = TRY(Utils::try_find(node_graph.nodes, node_id, "Invalid NodeID")).get();
    Node& node = TRY(Utils::try_find(NodePlot::node_map, node_input_storage.type_id, "Invalid NodeTypeId")).get();

    TRY(node.evalulate(enpf, this, node_id, output_cache));

    return TRY(Utils::try_find(output_cache.outputs, output_id, "OutputId not found")).get();
}

ErrorOr<Data> EvaluatedNodeGraph::get_input_data(EvaluatedNodePlotFile* enpf, NodeId node_id, InputId input_id) {
    NodeGraph::InputStorage& node_input_storage = TRY(Utils::try_find(node_graph.nodes, node_id, "Invalid NodeID")).get();
    auto& input_storage = TRY(Utils::try_find(node_input_storage.inputs, input_id, "Invalid InputID")).get();

    return std::visit(overloaded{
                          [&](Data data) -> ErrorOr<Data> { return data; },
                          [&](NodeGraph::InputPin input_pin) -> ErrorOr<Data> { return this->get_output_data(enpf, input_pin.node_id, input_pin.output_id); },
                      },
                      input_storage);
}

} // namespace NodePlot

MappedFile::~MappedFile() {
    if (munmap(data, file_size) == -1) {
        REQUIRE_NOT_REACHED("Failed to unmap CSV file");
    }
}

ErrorOr<NodeOutput> EvaluatedNodeGraph::get_output(GlobalNodeId id, OutputIndex output_id) {
    auto& loaded_node = loaded_nodes[id];

    if (auto val = loaded_node.cache.find(output_id); val != loaded_node.cache.end()) {
        return val->second;
    }

    auto node_or_error = node_file.node_graph(id.graph_name).nodes.find(id.id);
    if (node_or_error == node_file.node_graph(id.graph_name).nodes.end()) {
        return ERR("Trying to get output from non-existent node");
    }

    auto new_cache_or_error = std::visit([&](const auto& node) { return node_output(this, id, node); }, node_or_error->second);
    if (!new_cache_or_error.has_value()) {
        loaded_node.error_message = new_cache_or_error.error();
    } else {
        loaded_node.cache = new_cache_or_error.value();
    }

    if (auto val = loaded_node.cache.find(output_id); val != loaded_node.cache.end()) {
        return val->second;
    }

    return ERR("Requested output is not exported by the node");
}

ErrorOr<NodePlotFile> NodePlotFile::read_from_json(const nlohmann::json& json) {
    NodePlotFile res;
    for (auto it = json["node_graphs"].begin(); it != json["node_graphs"].end(); ++it) {
        std::string name = it.key();
        NodeGraph ng = it.value();
        res.m_node_graphs[it.key()] = it.value();
    }
    return res;
}

ErrorOr<void> NodePlotFile::save(std::filesystem::path path) {
    std::ofstream out(path);
    if (!out)
        return ERR("Could not open file for writing");

    nlohmann::json json = *this;

    out << json.dump(2);
    out.close();

    m_file_path = path;

    return {};
}

ErrorOr<void> NodePlotFile::add_node(std::string graph_name, Node node) {
    std::visit(
        [&](auto& node) {
            node.id = m_node_graphs[graph_name].next_node_id++;
            m_node_graphs[graph_name].nodes.insert({node.id, node});
        },
        node);
    return {};
}

ErrorOr<NodePlotFile> NodePlotFile::read(std::filesystem::path path) {
    std::ifstream f(path);
    if (!f)
        return ERR("Could not open input json file");

    nlohmann::json data = nlohmann::json::parse(f);

    auto res = TRY(read_from_json(data));
    res.m_file_path = std::filesystem::path(path);

    return res;
}

ErrorOr<NodePlotFile> NodePlotFile::create(std::filesystem::path path) {
    NodePlotFile res;
    res.m_file_path = std::filesystem::path(path);
    res.m_node_graphs = {};
    res.m_node_graphs.insert({MAIN_GRAPH_INDEX, NodeGraph{}});
    return res;
};

ErrorOr<double> string_to_double(std::string_view s) {
    double dbl;
    auto result = std::from_chars(s.data(), s.data() + s.size(), dbl);
    if (result.ec == std::errc()) {
        return dbl;
    }
    return ERR("Invalid number");
}

ErrorOr<ColumnNumeric> EvaluatedNodeGraph::column_as_numeric(Column column) {
    return std::visit(overloaded{
                          [&](ColumnNumeric& col) -> ErrorOr<ColumnNumeric> { return col; },
                          [&](ColumnCSVImported& col) -> ErrorOr<ColumnNumeric> {
                              ColumnNumeric res;
                              res.values.reserve(col.values.size());
                              for (auto& v : col.values) {
                                  auto parsed = string_to_double(v);
                                  if (parsed.has_value())
                                      res.values.push_back(*parsed);
                                  else
                                      res.values.push_back(std::numeric_limits<double>::quiet_NaN());
                              }
                              return res;
                          },
                      },
                      column.raw_column);
}

ErrorOr<std::vector<GlobalNodeId>> EvaluatedNodeGraph::dependent_nodes(GlobalNodeId gid) {

    auto n = node_file.node_graph(gid.graph_name).nodes.find(gid.id);
    if (n == node_file.node_graph(gid.graph_name).nodes.end())
        return ERR("Invalid Node ID");

    std::vector<GlobalNodeId> res;

    std::visit(
        [&](auto node) {
            std::apply(
                [&](auto&&... args) {
                    (overloaded{
                         [&]<typename T>(std::vector<T> list) {
                             for (auto& e : list) {
                                 overloaded{
                                     [&]<typename V>(Input<V> input) {
                                         if (std::holds_alternative<InputPin<V>>(input)) {
                                             LocalNodeId id = std::get<InputPin<V>>(input).node;
                                             if (id >= 0)
                                                 res.push_back(GlobalNodeId{
                                                     .id = id,
                                                     .graph_name = gid.graph_name,
                                                 });
                                         }
                                     },
                                     [&]<typename V>(InputPin<V> pin) {
                                         if (pin.node >= 0)
                                             res.push_back(GlobalNodeId{
                                                 pin.node,
                                                 gid.graph_name,
                                             });
                                     },
                                 }(e);
                             }
                         },
                         [&]<typename T>(Input<T> input) {
                             if (std::holds_alternative<InputPin<T>>(input)) {
                                 LocalNodeId id = std::get<InputPin<T>>(input).node;
                                 if (id >= 0)
                                     res.push_back(GlobalNodeId{
                                         .id = id,
                                         .graph_name = gid.graph_name,
                                     });
                             }
                         },
                         [&]<typename T>(InputPin<T> pin) {
                             if (pin.node >= 0)
                                 res.push_back(GlobalNodeId{
                                     pin.node,
                                     gid.graph_name,
                                 });
                         },
                         [&]<typename T>(T input) {},
                     }(std::get<NODE_INPUT_INDEX_STORAGE>(args))

                         ,
                     ...);
                },
                node.inputs());
        },
        n->second);

    return res;
}
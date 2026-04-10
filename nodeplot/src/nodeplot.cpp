#include "nodeplot.h"
#include "nlohmann/json_fwd.hpp"

#include <limits>
#include <sys/mman.h>

#include <charconv>
#include <fstream>
#include <variant>

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

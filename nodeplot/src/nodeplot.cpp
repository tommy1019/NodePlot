#include "nodeplot.h"

#include <sys/mman.h>

#include <charconv>
#include <fstream>

MappedFile::~MappedFile() {
    if (munmap(data, file_size) == -1) {
        REQUIRE_NOT_REACHED("Failed to unmap CSV file");
    }
}

ErrorOr<NodeOutput> EvaluatedNodeGraph::get_output(NodeId node_id, OutputId output_id) {
    auto& loaded_node = loaded_nodes[node_id];

    if (auto val = loaded_node.cache.find(output_id); val != loaded_node.cache.end()) {
        return val->second;
    }

    auto node_or_error = node_graph.nodes().find(node_id);
    if (node_or_error == node_graph.nodes().end()) {
        return ERR("Trying to get output from non-existent node");
    }

    // TODO: Make this function not take the output id and just have the node generate all outputs then just take from the cache
    auto node_result = std::visit([&](const auto& node) { return node_output(this, node, output_id); }, node_or_error->second);

    if (node_result.has_value()) {
        loaded_node.cache[output_id] = *node_result;
        return loaded_node.cache[output_id];
    } else {
        loaded_node.error_message = node_result.error();
        return node_result;
    }
}

ErrorOr<NodeGraph> NodeGraph::read_from_json(const nlohmann::json& json) {
    NodeGraph res;

    for (auto& node_pair : json["nodes"]) {
        NodeId id = node_pair[0];
        res.m_next_node_id = std::max(id + 1, res.m_next_node_id);
        auto& node_json = node_pair[1];

        auto& type = node_json["type"];

        std::optional<Node> parsed_node;

        for_each_type<Node>([&]<typename T>() {
            if (type == T::type()) {
                if (parsed_node.has_value())
                    REQUIRE_NOT_REACHED("Error while parsing json. Node matched two types");
                T typed_node = node_json;
                parsed_node = typed_node;
            }
        });

        if (!parsed_node.has_value())
            return ERR("Failed to parse node: unknown type");

        res.m_nodes[id] = *parsed_node;
    }

    res.m_visualize_node = json["visualize_node"];

    return res;
}

ErrorOr<void> NodeGraph::save(std::filesystem::path path) {
    std::ofstream out(path);
    if (!out)
        return ERR("Could not open file for writing");

    out << TRY(json_string());
    out.close();

    m_file_path = path;

    return {};
}

ErrorOr<std::string> NodeGraph::json_string() {
    nlohmann::json json;

    std::map<NodeId, nlohmann::json> node_jsons;
    for (auto n : m_nodes) {
        node_jsons[n.first] = std::visit(
            [](auto& n) -> nlohmann::json {
                nlohmann::json res = n;

                res["type"] = std::remove_reference_t<decltype(n)>::type();

                return res;
            },
            n.second);
    }

    json["nodes"] = node_jsons;
    json["visualize_node"] = m_visualize_node;

    return json.dump(2);
}

ErrorOr<void> NodeGraph::add_node(Node node) {
    std::visit(
        [&](auto& node) {
            node.id = m_next_node_id++;
            m_nodes.insert({node.id, node});
        },
        node);
    return {};
}

ErrorOr<NodeGraph> NodeGraph::read(std::filesystem::path path) {
    std::ifstream f(path);
    if (!f)
        return ERR("Could not open input json file");

    nlohmann::json data = nlohmann::json::parse(f);

    auto res = TRY(read_from_json(data));
    res.m_file_path = std::filesystem::path(path);

    return res;
}

ErrorOr<NodeGraph> NodeGraph::create(std::filesystem::path path) {
    NodeGraph res;
    res.m_file_path = std::filesystem::path(path);

    return res;
};

void Column::ensure_numeric() {
    if (numeric_values.has_value())
        return;

    auto to_double = [](std::string_view s) {
        double dbl;
        auto result = std::from_chars(s.data(), s.data() + s.size(), dbl);
        if (result.ec == std::errc()) {
            return dbl;
        }
        return std::numeric_limits<double>::quiet_NaN();
    };

    numeric_values = std::vector<double>{};
    for (auto& str : values) {
        numeric_values->push_back(to_double(str));
    }
}

ErrorOr<double> string_to_double(std::string_view s) {
    double dbl;
    auto result = std::from_chars(s.data(), s.data() + s.size(), dbl);
    if (result.ec == std::errc()) {
        return dbl;
    }
    return ERR("Invalid number");
}

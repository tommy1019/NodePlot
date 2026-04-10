#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "error.h"
#include "utils.h"

using GraphIndex = std::string;
using OutputIndex = std::string;

using LocalNodeId = int64_t;
struct GlobalNodeId {
    LocalNodeId id;
    GraphIndex graph_name;

    auto operator<=>(const GlobalNodeId& other) const = default;
};

template <typename T>
struct InputPin {
    LocalNodeId node = -1;
    OutputIndex output_index = "";

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(InputPin, node, output_index);
};

ErrorOr<double> string_to_double(std::string_view s);

template <typename T>
using Input = std::variant<T, InputPin<T>>;
template <typename T>
void to_json(nlohmann::json& j, const Input<T>& p) {
    j = nlohmann::json{};
    if (std::holds_alternative<T>(p))
        j["type"] = "value";
    else
        j["type"] = "input";
    j["value"] = std::visit([](auto& v) -> nlohmann::json { return v; }, p);
}
template <typename T>
void from_json(const nlohmann::json& j, Input<T>& p) {
    if (j["type"] == "value") {
        T v;
        j.at("value").get_to(v);
        p = v;
    } else if (j["type"] == "input") {
        InputPin<T> v;
        j.at("value").get_to(v);
        p = v;
    } else
        p = {};
}

struct Color {
    float r, g, b, a;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Color, r, g, b, a);
};

struct MappedFile {
    char* data;
    size_t file_size;
    std::string_view full_string_view;
    ~MappedFile();
};

struct ColumnName {
    std::string name;

    friend void to_json(nlohmann::json& json, const ColumnName& c) { json = c.name; }
    friend void from_json(const nlohmann::json& json, ColumnName& c) { json.get_to(c.name); }
};

struct ColumnCSVImported {
    std::shared_ptr<MappedFile> mapped_file;
    std::vector<std::string_view> values;
};

struct ColumnNumeric {
    std::vector<double> values;
};

struct Column {
    using ValueType = std::variant<std::string_view, double>;
    std::variant<ColumnCSVImported, ColumnNumeric> raw_column;

    size_t size() {
        return std::visit([](auto c) { return c.values.size(); }, raw_column);
    }

    ValueType operator[](size_t index) {
        return std::visit([&](auto c) -> ValueType { return c.values[index]; }, raw_column);
    }
};

struct ScatterSeries {
    ColumnNumeric x;
    ColumnNumeric y;

    Color color;
    double point_size = 1;
};

struct LineSeries {
    ColumnNumeric x;
    ColumnNumeric y;

    Color color;
    double stroke_width = 1;
};

struct RibbonSeries {
    ColumnNumeric x;
    ColumnNumeric y_min;
    ColumnNumeric y_max;

    Color color;
};

using Series = std::variant<ScatterSeries, LineSeries, RibbonSeries>;

struct Table {
    std::map<std::string, Column> columns;
    std::vector<std::string> column_names;
};

struct Margins {
    float left;
    float right;
    float top;
    float bottom;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Margins, top, bottom, left, right);
};

struct GraphStyle {
    Margins plot_margines;
    Margins internal_plot_margines;

    double x_axis_tick_mark_font_size;
    double x_axis_tick_mark_size;
    double x_axis_label_font_size;

    double y_axis_tick_mark_font_size;
    double y_axis_tick_mark_size;
    double y_axis_label_font_size;

    double title_font_size;
};

struct Graph {
    std::vector<Series> series;
    std::string title;
    std::string x_lab;
    std::string y_lab;
    bool x_log;
    bool y_log;
    GraphStyle style;
};

struct EvaluatedNodeGraph;

using NodeOutput = std::variant<Table, ColumnName, Column, Series, GraphStyle, Graph, std::string, std::filesystem::path, Margins, Color, bool, double, int32_t>;

constexpr size_t NODE_INPUT_INDEX_STORAGE = 0;
constexpr size_t NODE_INPUT_INDEX_ID = 1;
constexpr size_t NODE_INPUT_INDEX_NAME = 2;
constexpr size_t NODE_INPUT_INDEX_ENUM_OPTIONS = 3;

constexpr size_t NODE_OUTPUT_INDEX_ID = 0;
constexpr size_t NODE_OUTPUT_INDEX_NAME = 1;

struct BaseNode {
    LocalNodeId id;

    std::pair<float, float> pos = {};
};

#include "nodes.h"

using Node = std::variant<InputNode,
                          OutputNode,
                          IntegerValueNode,
                          NumericValueNode,
                          StringValueNode,
                          CSVImportNode,
                          FilterTableNode,
                          ColumnSelectNode,
                          BinaryOperationNode,
                          SampledPropertyExtractNode,
                          ScatterSeriesCreateNode,
                          LineSeriesCreateNode,
                          RibbonSeriesCreateNode,
                          CreateGraphStyleNode,
                          CreateGraphNode>;

constexpr GraphIndex MAIN_GRAPH_INDEX = "main";

constexpr LocalNodeId NODE_ID_INPUT = 0;
constexpr LocalNodeId NODE_ID_OUTPUT = 1;

constexpr GlobalNodeId GLOBAL_NODE_ID_INPUT = {.id = NODE_ID_INPUT, .graph_name = MAIN_GRAPH_INDEX};
constexpr GlobalNodeId GLOBAL_NODE_ID_OUTPUT = {.id = NODE_ID_OUTPUT, .graph_name = MAIN_GRAPH_INDEX};

struct NodeGraph {
    LocalNodeId next_node_id;
    std::map<LocalNodeId, Node> nodes;

    NodeGraph() {
        next_node_id = 2;
        nodes = {
            {NODE_ID_INPUT, Node(InputNode{})},
            {NODE_ID_OUTPUT, Node(OutputNode{})},
        };

        std::get<InputNode>(nodes[NODE_ID_INPUT]).id = NODE_ID_INPUT;
        std::get<OutputNode>(nodes[NODE_ID_OUTPUT]).id = NODE_ID_OUTPUT;

        std::get<OutputNode>(nodes[NODE_ID_OUTPUT]).pos = {300, 300};
        printf("Populating nodes...\n");
    }

    friend void to_json(nlohmann::json& json, const NodeGraph& n) {
        std::map<LocalNodeId, nlohmann::json> node_jsons;
        for (auto n : n.nodes) {
            node_jsons[n.first] = std::visit(
                [](auto& n) -> nlohmann::json {
                    nlohmann::json res = n;
                    res["type"] = std::remove_reference_t<decltype(n)>::type();
                    return res;
                },
                n.second);
        }

        json["nodes"] = node_jsons;
    }
    friend void from_json(const nlohmann::json& json, NodeGraph& n) {
        for (auto& node_pair : json["nodes"]) {
            LocalNodeId id = node_pair[0];
            n.next_node_id = std::max(id + 1, n.next_node_id);
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

            n.nodes[id] = parsed_node.value();
        }
    }
};

struct NodePlotFile {
    static ErrorOr<NodePlotFile> create(std::filesystem::path path);
    static ErrorOr<NodePlotFile> read(std::filesystem::path path);

    ErrorOr<void> save(std::filesystem::path path);
    ErrorOr<void> save() { return save(m_file_path); }

    ErrorOr<void> add_node(std::string graph_name, Node node);

    std::map<std::string, NodeGraph>& node_graphs() { return m_node_graphs; }
    const std::map<std::string, NodeGraph>& node_graphs() const { return m_node_graphs; }

    NodeGraph& node_graph(std::string name) { return m_node_graphs[name]; }
    const NodeGraph& node_graph(std::string name) const { return m_node_graphs.at(name); }

    NodeGraph& main_graph() { return m_node_graphs[MAIN_GRAPH_INDEX]; }
    const NodeGraph& main_graph() const { return m_node_graphs.at(MAIN_GRAPH_INDEX); }

    const std::filesystem::path& file_path() const { return m_file_path; }

    friend void to_json(nlohmann::json& json, const NodePlotFile& n) { json["node_graphs"] = n.m_node_graphs; }
    friend void from_json(const nlohmann::json& json, NodePlotFile& n) { n.m_node_graphs = json["node_graphs"]; }

  private:
    NodePlotFile() = default;

    std::map<std::string, NodeGraph> m_node_graphs;

    std::filesystem::path m_file_path;

    static ErrorOr<NodePlotFile> read_from_json(const nlohmann::json& json);
};

struct EvaluatedNodeGraph;

ErrorOr<std::map<OutputIndex, NodeOutput>> node_output(EvaluatedNodeGraph* graph, GlobalNodeId id, const auto& node);

struct EvaluatedNodeGraph {
    NodePlotFile node_file;

    struct LoadedNode {
        std::map<OutputIndex, NodeOutput> cache;
        std::optional<std::string> error_message;
    };

    std::map<GlobalNodeId, LoadedNode> loaded_nodes;

    ErrorOr<std::vector<GlobalNodeId>> dependent_nodes(GlobalNodeId gid);

    template <typename T>
    ErrorOr<T> get_input(GraphIndex graph, const InputPin<T>& input) {
        auto out = TRY(get_output(GlobalNodeId{.id = input.node, .graph_name = graph}, input.output_index));

        if constexpr (std::is_same_v<T, std::filesystem::path>) { // TODO: Don't like this manual check and conversion from paths to strings
            if (std::holds_alternative<std::string>(out))
                return std::get<std::string>(out);
        }

        if constexpr (std::is_same_v<T, ColumnName>) { // TODO: Don't like this manual check and conversion from ColumnName to strings
            if (std::holds_alternative<std::string>(out))
                return ColumnName{std::get<std::string>(out)};
        }

        if (!std::holds_alternative<T>(out)) {
            return ERR("Input has incorrect type");
        }
        return std::get<T>(out);
    }

    template <typename T>
    ErrorOr<T> get_input(GraphIndex graph, const Input<T>& input) {
        if (std::holds_alternative<T>(input))
            return std::get<T>(input);
        return get_input(graph, std::get<InputPin<T>>(input));
    }

    ErrorOr<NodeOutput> get_output(GlobalNodeId node_id, OutputIndex output_id);

    ErrorOr<ColumnNumeric> column_as_numeric(Column column);
};

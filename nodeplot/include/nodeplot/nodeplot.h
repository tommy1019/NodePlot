#pragma once

#include <cstdint>
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

using NodeId = int64_t;
using OutputId = int64_t;

template <typename T>
struct InputPin {
    NodeId node = -1;
    OutputId output_index = -1;

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
    if (j["type"] == "value")
        j.at("value").get_to(std::get<T>(p));
    else if (j["type"] == "input")
        j.at("value").get_to(std::get<InputPin<T>>(p));
    else
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

// TODO: Use shared_ptr to maybe make the column storage copy on write
struct Column {
    std::shared_ptr<MappedFile> mapped_file;
    std::string name;
    std::vector<std::string_view> values;
    std::optional<std::vector<double>> numeric_values;

    void ensure_numeric();
};

struct ScatterSeries {
    Column x;
    Column y;

    Color color = {1, 0, 0, 1};
    double point_size = 1;
};

struct LineSeries {
    Column x;
    Column y;

    Color color = {1, 0, 0, 1};
    double stroke_width = 1;
};

struct RibbonSeries {
    Column x;
    Column y_min;
    Column y_max;

    Color color = {1, 0, 0, 0.2};
};

using Series = std::variant<ScatterSeries, LineSeries, RibbonSeries>;

struct Table {
    std::shared_ptr<MappedFile> mapped_file;
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

using NodeOutput = std::variant<Table, Column, Series, GraphStyle, Graph, std::string, std::filesystem::path, Margins, Color, bool, double, int32_t>;

struct BaseNode {
    NodeId id;

    std::pair<float, float> pos = {};
};

#define DEPENDENT_NODE_LIST_START                                                                                                                                                                      \
    std::vector<NodeId> dependent_nodes() {                                                                                                                                                            \
        std::vector<NodeId> res;

#define DEPENDENT_NODE_LIST_INPUT(type, name)                                                                                                                                                          \
    if (std::holds_alternative<InputPin<type>>(name) && std::get<InputPin<type>>(name).node >= 0)                                                                                                      \
        res.push_back(std::get<InputPin<type>>(name).node);

#define DEPENDENT_NODE_LIST_INPUT_PIN(name)                                                                                                                                                            \
    if (name.node >= 0)                                                                                                                                                                                \
        res.push_back(name.node);

#define DEPENDENT_NODE_LIST_END                                                                                                                                                                        \
    return res;                                                                                                                                                                                        \
    }

struct CSVImportNode : public BaseNode {
    Input<std::filesystem::path> i_source_path;
    Input<bool> i_has_headers;

    OutputId output_id;

    static std::string name() { return "CSV Input"; }
    static std::string type() { return "csv_import"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CSVImportNode, id, pos, i_source_path, i_has_headers, output_id);

    DEPENDENT_NODE_LIST_START
    DEPENDENT_NODE_LIST_INPUT(std::filesystem::path, i_source_path);
    DEPENDENT_NODE_LIST_INPUT(bool, i_has_headers);
    DEPENDENT_NODE_LIST_END
};

struct FilterTableNode : public BaseNode {
    InputPin<Table> i_table;
    Input<std::string> i_column_name;
    Input<int32_t> i_compare_type;
    Input<std::string> i_compare_value;
    Input<bool> i_numeric_compare;

    static std::string name() { return "Filter Table"; }
    static std::string type() { return "filter_table"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(FilterTableNode, id, pos, i_table, i_column_name, i_compare_type, i_compare_value, i_numeric_compare);

    DEPENDENT_NODE_LIST_START
    DEPENDENT_NODE_LIST_INPUT_PIN(i_table)
    DEPENDENT_NODE_LIST_INPUT(std::string, i_column_name);
    DEPENDENT_NODE_LIST_INPUT(int32_t, i_compare_type);
    DEPENDENT_NODE_LIST_INPUT(std::string, i_compare_value);
    DEPENDENT_NODE_LIST_INPUT(bool, i_numeric_compare);
    DEPENDENT_NODE_LIST_END
};

struct ColumnSelectNode : public BaseNode {
    InputPin<Table> i_table;
    Input<std::string> i_column_name;

    static std::string name() { return "Column Select"; }
    static std::string type() { return "column_select"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ColumnSelectNode, id, pos, i_table, i_column_name);

    DEPENDENT_NODE_LIST_START
    DEPENDENT_NODE_LIST_INPUT_PIN(i_table)
    DEPENDENT_NODE_LIST_INPUT(std::string, i_column_name);
    DEPENDENT_NODE_LIST_END
};

struct SampledPropertyExtractNode : public BaseNode {
    InputPin<Column> i_x;
    InputPin<Column> i_y;

    static std::string name() { return "Sampled Property Extract"; }
    static std::string type() { return "sampled_property_extract"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SampledPropertyExtractNode, id, pos, i_x, i_y);

    DEPENDENT_NODE_LIST_START
    DEPENDENT_NODE_LIST_INPUT_PIN(i_x)
    DEPENDENT_NODE_LIST_INPUT_PIN(i_y)
    DEPENDENT_NODE_LIST_END
};

struct ScatterSeriesCreateNode : public BaseNode {
    InputPin<Column> i_x;
    InputPin<Column> i_y;
    Input<Color> i_color;
    Input<double> i_point_size;

    static std::string name() { return "Scatter Series Create"; }
    static std::string type() { return "scatter_series_create"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ScatterSeriesCreateNode, id, pos, i_x, i_y, i_color, i_point_size);

    DEPENDENT_NODE_LIST_START
    DEPENDENT_NODE_LIST_INPUT_PIN(i_x)
    DEPENDENT_NODE_LIST_INPUT_PIN(i_y)
    DEPENDENT_NODE_LIST_INPUT(Color, i_color)
    DEPENDENT_NODE_LIST_INPUT(double, i_point_size)
    DEPENDENT_NODE_LIST_END
};

struct LineSeriesCreateNode : public BaseNode {
    InputPin<Column> i_x;
    InputPin<Column> i_y;
    Input<Color> i_color;
    Input<double> i_stroke_width;

    static std::string name() { return "Line Series Create"; }
    static std::string type() { return "line_series_create"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LineSeriesCreateNode, id, pos, i_x, i_y, i_color, i_stroke_width);

    DEPENDENT_NODE_LIST_START
    DEPENDENT_NODE_LIST_INPUT_PIN(i_x)
    DEPENDENT_NODE_LIST_INPUT_PIN(i_y)
    DEPENDENT_NODE_LIST_INPUT(Color, i_color)
    DEPENDENT_NODE_LIST_INPUT(double, i_stroke_width)
    DEPENDENT_NODE_LIST_END
};

struct RibbonSeriesCreateNode : public BaseNode {
    InputPin<Column> i_x;
    InputPin<Column> i_y_min;
    InputPin<Column> i_y_max;
    Input<Color> i_color;

    static std::string name() { return "Ribbon Series Create"; }
    static std::string type() { return "ribbon_series_create"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(RibbonSeriesCreateNode, id, pos, i_x, i_y_min, i_y_max, i_color);

    DEPENDENT_NODE_LIST_START
    DEPENDENT_NODE_LIST_INPUT_PIN(i_x)
    DEPENDENT_NODE_LIST_INPUT_PIN(i_y_min)
    DEPENDENT_NODE_LIST_INPUT_PIN(i_y_max)
    DEPENDENT_NODE_LIST_INPUT(Color, i_color)
    DEPENDENT_NODE_LIST_END
};

struct CreateGraphStyleNode : public BaseNode {
    Input<Margins> i_plot_margines = Margins{.left = 10, .right = 10, .top = 10, .bottom = 10};
    Input<Margins> i_internal_plot_margines = Margins{.left = 0, .right = 0, .top = 0, .bottom = 0};

    Input<double> i_x_axis_tick_mark_font_size = 12.0;
    Input<double> i_x_axis_tick_mark_size = 12.0;
    Input<double> i_x_axis_label_font_size = 12.0;

    Input<double> i_y_axis_tick_mark_font_size = 12.0;
    Input<double> i_y_axis_tick_mark_size = 12.0;
    Input<double> i_y_axis_label_font_size = 12.0;

    Input<double> i_title_font_size = 16.0;

    static std::string name() { return "Create Graph Style"; }
    static std::string type() { return "create_graph_style"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(CreateGraphStyleNode,
                                                id,
                                                pos,
                                                i_plot_margines,
                                                i_internal_plot_margines,
                                                i_x_axis_tick_mark_font_size,
                                                i_x_axis_tick_mark_size,
                                                i_x_axis_label_font_size,
                                                i_y_axis_tick_mark_font_size,
                                                i_y_axis_tick_mark_size,
                                                i_y_axis_label_font_size,
                                                i_title_font_size);

    DEPENDENT_NODE_LIST_START
    DEPENDENT_NODE_LIST_INPUT(Margins, i_plot_margines)
    DEPENDENT_NODE_LIST_INPUT(Margins, i_internal_plot_margines)
    DEPENDENT_NODE_LIST_INPUT(double, i_x_axis_tick_mark_font_size)
    DEPENDENT_NODE_LIST_INPUT(double, i_x_axis_tick_mark_size)
    DEPENDENT_NODE_LIST_INPUT(double, i_x_axis_label_font_size)
    DEPENDENT_NODE_LIST_INPUT(double, i_y_axis_tick_mark_font_size)
    DEPENDENT_NODE_LIST_INPUT(double, i_y_axis_tick_mark_size)
    DEPENDENT_NODE_LIST_INPUT(double, i_y_axis_label_font_size)
    DEPENDENT_NODE_LIST_INPUT(double, i_title_font_size)
    DEPENDENT_NODE_LIST_END
};

struct CreateGraphNode : public BaseNode {
    std::vector<InputPin<Series>> i_series;

    Input<std::string> i_title;

    Input<std::string> i_xlab;
    Input<std::string> i_ylab;

    Input<bool> i_x_axis_log_scale = false;
    Input<bool> i_y_axis_log_scale = false;

    InputPin<GraphStyle> i_style;

    static std::string name() { return "Create Graph"; }
    static std::string type() { return "create_graph"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CreateGraphNode, id, pos, i_series, i_title, i_xlab, i_ylab, i_x_axis_log_scale, i_y_axis_log_scale, i_style);

    DEPENDENT_NODE_LIST_START
    for (auto& p : i_series)
        DEPENDENT_NODE_LIST_INPUT_PIN(p)
    DEPENDENT_NODE_LIST_INPUT(std::string, i_title)
    DEPENDENT_NODE_LIST_INPUT(std::string, i_xlab)
    DEPENDENT_NODE_LIST_INPUT(std::string, i_ylab)
    DEPENDENT_NODE_LIST_INPUT(bool, i_x_axis_log_scale)
    DEPENDENT_NODE_LIST_INPUT(bool, i_y_axis_log_scale)
    DEPENDENT_NODE_LIST_INPUT_PIN(i_style)
    DEPENDENT_NODE_LIST_END
};

struct OutputNode : public BaseNode {
    std::vector<InputPin<Graph>> i_graphs;

    Input<std::string> i_output_filename;

    Input<double> i_width = 300.0;
    Input<double> i_height = 300.0;

    Input<int32_t> i_grid_cols = 1;
    Input<int32_t> i_grid_rows = 1;

    static std::string name() { return "Output"; }
    static std::string type() { return "output"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OutputNode, id, pos, i_graphs, i_output_filename, i_width, i_height, i_grid_cols, i_grid_rows);

    DEPENDENT_NODE_LIST_START
    for (auto& pin : i_graphs)
        DEPENDENT_NODE_LIST_INPUT_PIN(pin)
    DEPENDENT_NODE_LIST_INPUT(std::string, i_output_filename)
    DEPENDENT_NODE_LIST_INPUT(double, i_width)
    DEPENDENT_NODE_LIST_INPUT(double, i_height)
    DEPENDENT_NODE_LIST_INPUT(int32_t, i_grid_cols)
    DEPENDENT_NODE_LIST_INPUT(int32_t, i_grid_rows)
    DEPENDENT_NODE_LIST_END

    ErrorOr<std::string> get_svg(EvaluatedNodeGraph* graph);
};

using Node = std::variant<CSVImportNode,
                          FilterTableNode,
                          ColumnSelectNode,
                          SampledPropertyExtractNode,
                          ScatterSeriesCreateNode,
                          LineSeriesCreateNode,
                          RibbonSeriesCreateNode,
                          CreateGraphStyleNode,
                          CreateGraphNode,
                          OutputNode>;

struct NodeGraph {

    static ErrorOr<NodeGraph> create(std::filesystem::path path);
    static ErrorOr<NodeGraph> read(std::filesystem::path path);

    ErrorOr<void> save(std::filesystem::path path);
    ErrorOr<void> save() { return save(m_file_path); }

    ErrorOr<std::string> json_string();

    ErrorOr<void> add_node(Node node);

    std::map<NodeId, Node>& nodes() { return m_nodes; }
    const std::map<NodeId, Node>& nodes() const { return m_nodes; }

    NodeId& visualize_node() { return m_visualize_node; }
    const NodeId& visualize_node() const { return m_visualize_node; }

    const std::filesystem::path& file_path() const { return m_file_path; }

  private:
    NodeGraph() = default;

    NodeId m_next_node_id = 0;
    std::map<NodeId, Node> m_nodes;

    NodeId m_visualize_node = -1;

    std::filesystem::path m_file_path;

    static ErrorOr<NodeGraph> read_from_json(const nlohmann::json& json);
};

struct EvaluatedNodeGraph;

ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const auto& node, OutputId id);

struct EvaluatedNodeGraph {
    NodeGraph node_graph;

    struct LoadedNode {
        std::map<OutputId, NodeOutput> cache;

        std::optional<std::string> error_message;
    };

    std::map<NodeId, LoadedNode> loaded_nodes;

    template <typename T>
    ErrorOr<T> get_input(const Input<T>& input) {
        if (std::holds_alternative<T>(input))
            return std::get<T>(input);
        auto out = TRY(get_output(std::get<InputPin<T>>(input)));
        if (!std::holds_alternative<T>(out)) {
            return ERR("Input has incorrect type");
        }
        return std::get<T>(out);
    }

    template <typename T>
    ErrorOr<T> get_input(const InputPin<T>& input) {
        auto out = TRY(get_output(input));
        if (!std::holds_alternative<T>(out)) {
            return ERR("Input has incorrect type");
        }
        return std::get<T>(out);
    }

    ErrorOr<NodeOutput> get_output(NodeId node_id, OutputId output_id);

    template <typename T>
    ErrorOr<NodeOutput> get_output(InputPin<T> pin) {
        // TODO: Check return type?
        return get_output(pin.node, pin.output_index);
    }
};

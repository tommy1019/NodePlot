#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "error.h"
#include "utils.h"

using NodeId = int64_t;
using OutputIndex = int64_t;

template <typename T>
struct InputPin {
    NodeId node = -1;
    OutputIndex output_index = -1;

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

struct BaseNode {
    NodeId id;

    std::pair<float, float> pos = {};
};

struct IntegerValueNode : public BaseNode {
    Input<int32_t> i_val;

    static std::string name() { return "Integer Value"; }
    static std::string type() { return "int_value"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(IntegerValueNode, id, pos, i_val);

    constexpr auto inputs() { return std::make_tuple(std::make_tuple(std::reference_wrapper{i_val}, "value", "Value")); }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "value", "Value")); }
};

struct NumericValueNode : public BaseNode {
    Input<double> i_val;

    static std::string name() { return "Numeric Value"; }
    static std::string type() { return "numeric_value"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(NumericValueNode, id, pos, i_val);

    constexpr auto inputs() { return std::make_tuple(std::make_tuple(std::reference_wrapper{i_val}, "value", "Value")); }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "value", "Value")); }
};

struct StringValueNode : public BaseNode {
    Input<std::string> i_val;

    static std::string name() { return "String Value"; }
    static std::string type() { return "string_value"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StringValueNode, id, pos, i_val);

    constexpr auto inputs() { return std::make_tuple(std::make_tuple(std::reference_wrapper{i_val}, "value", "Value")); }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "value", "Value")); }
};

struct CSVImportNode : public BaseNode {
    Input<std::filesystem::path> i_source_path;
    Input<bool> i_has_headers = true;

    static std::string name() { return "CSV Input"; }
    static std::string type() { return "csv_import"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CSVImportNode, id, pos, i_source_path, i_has_headers);

    constexpr auto inputs() {
        return std::make_tuple(std::make_tuple(std::reference_wrapper{i_source_path}, "source_path", "Source Path"),
                               std::make_tuple(std::reference_wrapper{i_has_headers}, "has_headers", "Has Headers"));
    }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "table_data", "Table Data")); }
};

struct FilterTableNode : public BaseNode {
    InputPin<Table> i_table;
    Input<ColumnName> i_column_name;
    Input<std::string> i_compare_type = "<";
    Input<std::string> i_compare_value;
    Input<bool> i_numeric_compare;

    static std::string name() { return "Filter Table"; }
    static std::string type() { return "filter_table"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(FilterTableNode, id, pos, i_table, i_column_name, i_compare_type, i_compare_value, i_numeric_compare);

    constexpr auto inputs() {
        return std::make_tuple(std::make_tuple(std::reference_wrapper{i_table}, "table", "Table"),
                               std::make_tuple(std::reference_wrapper{i_column_name}, "column_name", "Column Name"),
                               std::make_tuple(std::reference_wrapper{i_compare_type},
                                               "compare_type",
                                               "Compare Type",
                                               std::map<std::string, std::string>{
                                                   {"<", "< Less Than"},
                                                   {"<=", "<= Less Than or Equal To"},
                                                   {"==", "== Equals"},
                                                   {"!=", "!= Not Equals"},
                                                   {">=", ">= Greater Than or Equal To"},
                                                   {">", "> Greater Than"},
                                               }),
                               std::make_tuple(std::reference_wrapper{i_compare_value}, "compare_value", "Compare Value"),
                               std::make_tuple(std::reference_wrapper{i_numeric_compare}, "numeric_compare", "Numeric Compare"));
    }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "table", "Table")); }
};

struct ColumnSelectNode : public BaseNode {
    InputPin<Table> i_table;
    Input<ColumnName> i_column_name;

    static std::string name() { return "Column Select"; }
    static std::string type() { return "column_select"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ColumnSelectNode, id, pos, i_table, i_column_name);

    constexpr auto inputs() {
        return std::make_tuple(std::make_tuple(std::reference_wrapper{i_table}, "table", "Table"), std::make_tuple(std::reference_wrapper{i_column_name}, "column_name", "Column Name"));
    }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "column", "Column")); }
};

struct BinaryOperationNode : public BaseNode {
    // TODO: Allow the type system to specify these can be columns or values
    InputPin<Column> i_a;
    InputPin<Column> i_b;
    Input<std::string> i_operation = "+";

    static std::string name() { return "Binary Operation"; }
    static std::string type() { return "binary_operation"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BinaryOperationNode, id, pos, i_a, i_b, i_operation);

    constexpr auto inputs() {
        return std::make_tuple(std::make_tuple(std::reference_wrapper{i_a}, "a", "A"),
                               std::make_tuple(std::reference_wrapper{i_b}, "b", "B"),
                               std::make_tuple(std::reference_wrapper{i_operation},
                                               "operation",
                                               "Operation",
                                               std::map<std::string, std::string>{
                                                   {"+", "+ Addition"},
                                                   {"-", "- Subtraction"},
                                                   {"*", "* Multiplication"},
                                                   {"/", "/ Division"},
                                               }));
    }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "result", "Result")); }
};

struct SampledPropertyExtractNode : public BaseNode {
    InputPin<Column> i_x;
    InputPin<Column> i_y;

    static std::string name() { return "Sampled Property Extract"; }
    static std::string type() { return "sampled_property_extract"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SampledPropertyExtractNode, id, pos, i_x, i_y);

    constexpr auto inputs() { return std::make_tuple(std::make_tuple(std::reference_wrapper{i_x}, "x", "X"), std::make_tuple(std::reference_wrapper{i_y}, "y", "Y")); }

    constexpr auto outputs() {
        return std::make_tuple(std::make_tuple(OutputIndex{0}, "x", "X Values"),
                               std::make_tuple(OutputIndex{1}, "min", "Minimum"),
                               std::make_tuple(OutputIndex{2}, "avg", "Average"),
                               std::make_tuple(OutputIndex{3}, "stdev", "Standard Deviation"),
                               std::make_tuple(OutputIndex{4}, "max", "Maximum"),
                               std::make_tuple(OutputIndex{5}, "sample_count", "Sample Count"));
    }
};

struct ScatterSeriesCreateNode : public BaseNode {
    InputPin<Column> i_x;
    InputPin<Column> i_y;
    Input<Color> i_color = Color{.r = 1, .g = 0, .b = 0, .a = 1};
    Input<double> i_point_size = 3.0;

    static std::string name() { return "Scatter Series Create"; }
    static std::string type() { return "scatter_series_create"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ScatterSeriesCreateNode, id, pos, i_x, i_y, i_color, i_point_size);

    constexpr auto inputs() {
        return std::make_tuple(std::make_tuple(std::reference_wrapper{i_x}, "x", "X"),
                               std::make_tuple(std::reference_wrapper{i_y}, "y", "Y"),
                               std::make_tuple(std::reference_wrapper{i_color}, "color", "Color"),
                               std::make_tuple(std::reference_wrapper{i_point_size}, "point_size", "Point Size"));
    }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "series", "Series")); }
};

struct LineSeriesCreateNode : public BaseNode {
    InputPin<Column> i_x;
    InputPin<Column> i_y;
    Input<Color> i_color = Color{.r = 1, .g = 0, .b = 0, .a = 1};
    Input<double> i_stroke_width = 3.0;

    static std::string name() { return "Line Series Create"; }
    static std::string type() { return "line_series_create"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LineSeriesCreateNode, id, pos, i_x, i_y, i_color, i_stroke_width);

    constexpr auto inputs() {
        return std::make_tuple(std::make_tuple(std::reference_wrapper{i_x}, "x", "X"),
                               std::make_tuple(std::reference_wrapper{i_y}, "y", "Y"),
                               std::make_tuple(std::reference_wrapper{i_color}, "color", "Color"),
                               std::make_tuple(std::reference_wrapper{i_stroke_width}, "stroke_width", "Stroke Width"));
    }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "series", "Series")); }
};

struct RibbonSeriesCreateNode : public BaseNode {
    InputPin<Column> i_x;
    InputPin<Column> i_y_min;
    InputPin<Column> i_y_max;
    Input<Color> i_color = Color{.r = 1, .g = 0, .b = 0, .a = 0.25};

    static std::string name() { return "Ribbon Series Create"; }
    static std::string type() { return "ribbon_series_create"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(RibbonSeriesCreateNode, id, pos, i_x, i_y_min, i_y_max, i_color);

    constexpr auto inputs() {
        return std::make_tuple(std::make_tuple(std::reference_wrapper{i_x}, "x", "X"),
                               std::make_tuple(std::reference_wrapper{i_y_min}, "y_min", "Y Min"),
                               std::make_tuple(std::reference_wrapper{i_y_max}, "y_max", "Y Max"),
                               std::make_tuple(std::reference_wrapper{i_color}, "color", "Color"));
    }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "series", "Series")); }
};

struct CreateGraphStyleNode : public BaseNode {
    Input<Margins> i_plot_margines = Margins{.left = 60, .right = 10, .top = 10, .bottom = 50};
    Input<Margins> i_internal_margines = Margins{.left = 0, .right = 0, .top = 0, .bottom = 0};

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
                                                i_internal_margines,
                                                i_x_axis_tick_mark_font_size,
                                                i_x_axis_tick_mark_size,
                                                i_x_axis_label_font_size,
                                                i_y_axis_tick_mark_font_size,
                                                i_y_axis_tick_mark_size,
                                                i_y_axis_label_font_size,
                                                i_title_font_size);

    constexpr auto inputs() {
        return std::make_tuple(std::make_tuple(std::reference_wrapper{i_plot_margines}, "plot_margines", "Plot Margines"),
                               std::make_tuple(std::reference_wrapper{i_internal_margines}, "internal_margines", "Internal Margines"),

                               std::make_tuple(std::reference_wrapper{i_x_axis_tick_mark_font_size}, "x_axis_tick_mark_font_size", "X Axis Tick Mark Font Size"),
                               std::make_tuple(std::reference_wrapper{i_x_axis_tick_mark_size}, "x_axis_tick_mark_size", "X Axis Tick Mark Size"),
                               std::make_tuple(std::reference_wrapper{i_x_axis_label_font_size}, "x_axis_label_font_size", "X Axis Label Font Size"),

                               std::make_tuple(std::reference_wrapper{i_y_axis_tick_mark_font_size}, "y_axis_tick_mark_font_size", "Y Axis Tick Mark Font Size"),
                               std::make_tuple(std::reference_wrapper{i_y_axis_tick_mark_size}, "y_axis_tick_mark_size", "Y Axis Tick Mark Size"),
                               std::make_tuple(std::reference_wrapper{i_y_axis_label_font_size}, "y_axis_label_font_size", "Y Axis Label Font Size"),

                               std::make_tuple(std::reference_wrapper{i_title_font_size}, "title_font_size", "Title Font Size"));
    }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "graph_style", "Graph Style")); }
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

    constexpr auto inputs() {
        return std::make_tuple(std::make_tuple(std::reference_wrapper{i_series}, "series", "Series"),
                               std::make_tuple(std::reference_wrapper{i_title}, "title", "Title"),
                               std::make_tuple(std::reference_wrapper{i_xlab}, "xlab", "X Label"),
                               std::make_tuple(std::reference_wrapper{i_ylab}, "ylab", "X Label"),
                               std::make_tuple(std::reference_wrapper{i_x_axis_log_scale}, "x_axis_log_scale", "X Axis Log Scale"),
                               std::make_tuple(std::reference_wrapper{i_y_axis_log_scale}, "y_axis_log_scale", "Y Axis Log Scale"),
                               std::make_tuple(std::reference_wrapper{i_style}, "style", "Style"));
    }

    constexpr auto outputs() { return std::make_tuple(std::make_tuple(OutputIndex{0}, "graph", "Graph")); }
};

struct OutputNode : public BaseNode {
    std::vector<InputPin<Graph>> i_graphs;

    Input<std::string> i_output_filename;

    Input<double> i_width = 250.0;
    Input<double> i_height = 250.0;

    Input<int32_t> i_grid_cols = 1;
    Input<int32_t> i_grid_rows = 1;

    static std::string name() { return "Output"; }
    static std::string type() { return "output"; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OutputNode, id, pos, i_graphs, i_output_filename, i_width, i_height, i_grid_cols, i_grid_rows);

    constexpr auto inputs() {
        return std::make_tuple(std::make_tuple(std::reference_wrapper{i_graphs}, "graphs", "Graphs"),
                               std::make_tuple(std::reference_wrapper{i_output_filename}, "output_filename", "Output Filename"),
                               std::make_tuple(std::reference_wrapper{i_width}, "width", "Width"),
                               std::make_tuple(std::reference_wrapper{i_height}, "height", "Height"),
                               std::make_tuple(std::reference_wrapper{i_grid_cols}, "grid_cols", "Grid Columns"),
                               std::make_tuple(std::reference_wrapper{i_grid_rows}, "grid_rows", "Grid Rows"));
    }

    constexpr auto outputs() { return std::make_tuple(); }

    ErrorOr<std::string> get_svg(EvaluatedNodeGraph* graph);
};

using Node = std::variant<IntegerValueNode,
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

ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const auto& node, OutputIndex id);

struct EvaluatedNodeGraph {
    NodeGraph node_graph;

    struct LoadedNode {
        std::map<OutputIndex, NodeOutput> cache;

        std::optional<std::string> error_message;
    };

    std::map<NodeId, LoadedNode> loaded_nodes;

    ErrorOr<std::vector<NodeId>> dependent_nodes(NodeId id) {
        auto n = node_graph.nodes().find(id);
        if (n == node_graph.nodes().end())
            return ERR("Invalid Node ID");

        std::vector<NodeId> res;

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
                                                 NodeId id = std::get<InputPin<V>>(input).node;
                                                 if (id >= 0)
                                                     res.push_back(id);
                                             }
                                         },
                                         [&]<typename V>(InputPin<V> pin) {
                                             if (pin.node >= 0)
                                                 res.push_back(pin.node);
                                         },
                                     }(e);
                                 }
                             },
                             [&]<typename T>(Input<T> input) {
                                 if (std::holds_alternative<InputPin<T>>(input)) {
                                     NodeId id = std::get<InputPin<T>>(input).node;
                                     if (id >= 0)
                                         res.push_back(id);
                                 }
                             },
                             [&]<typename T>(InputPin<T> pin) {
                                 if (pin.node >= 0)
                                     res.push_back(pin.node);
                             },
                         }(std::get<0>(args))

                             ,
                         ...);
                    },
                    node.inputs());
            },
            n->second);

        return res;
    }

    template <typename T>
    ErrorOr<T> get_input(const InputPin<T>& input) {
        auto out = TRY(get_output(input));

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
    ErrorOr<T> get_input(const Input<T>& input) {
        if (std::holds_alternative<T>(input))
            return std::get<T>(input);
        return get_input(std::get<InputPin<T>>(input));
    }

    ErrorOr<NodeOutput> get_output(NodeId node_id, OutputIndex output_id);

    template <typename T>
    ErrorOr<NodeOutput> get_output(InputPin<T> pin) {
        return get_output(pin.node, pin.output_index);
    }

    ErrorOr<ColumnNumeric> column_as_numeric(Column column);
};

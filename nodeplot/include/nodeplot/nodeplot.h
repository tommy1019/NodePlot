#pragma once

#include <charconv>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "error.h"
#include "nlohmann/json_fwd.hpp"
#include "utils.h"

namespace NodePlot {

namespace Utils {

template <typename Map>
auto try_find(Map& map, auto key, std::string err) -> ErrorOr<std::reference_wrapper<typename Map::mapped_type>> {
    auto res = map.find(key);
    if (res == map.end())
        return ERR(err);
    return res->second;
}

} // namespace Utils

using GraphId = std::string;

using NodeTypeId = std::string;

using InputId = std::string;
using OutputId = std::string;

using NodeId = ssize_t;

struct MappedFile {
    char* data;
    size_t file_size;
    std::string_view full_string_view;
    ~MappedFile();
};

struct Color {
    float r, g, b, a;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Color, r, g, b, a);
};

struct Column {
    struct CSVImported {
        std::shared_ptr<MappedFile> mapped_file;
        std::vector<std::string_view> values;
    };

    struct Numeric {
        std::vector<double> values;
    };

    using ValueType = std::variant<std::string_view, double>;
    std::variant<CSVImported, Numeric> raw_column;

    size_t size() {
        return std::visit([](auto c) { return c.values.size(); }, raw_column);
    }

    ValueType operator[](size_t index) {
        return std::visit([&](auto c) -> ValueType { return c.values[index]; }, raw_column);
    }

    ErrorOr<Column::Numeric> as_numeric_column() {
        if (std::holds_alternative<Numeric>(raw_column))
            return std::get<Numeric>(raw_column);

        return std::visit(overloaded{
                              [](CSVImported& col) -> ErrorOr<Column::Numeric> {
                                  std::vector<double> res;
                                  res.reserve(col.values.size());

                                  for (auto& s : col.values) {
                                      float value;
                                      auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

                                      if (ec == std::errc()) {
                                          res.push_back(value);
                                      } else if (ec == std::errc::invalid_argument) {
                                          res.push_back(std::numeric_limits<float>::quiet_NaN());
                                      } else if (ec == std::errc::result_out_of_range) {
                                          res.push_back(std::numeric_limits<float>::quiet_NaN());
                                      }
                                  }

                                  return Column::Numeric{res};
                              },
                              [](Numeric& col) -> ErrorOr<Column::Numeric> { return col; },
                          },
                          raw_column);
    }
};

struct ScatterSeries {
    Column x;
    Column y;

    NodePlot::Color color;
    double point_size = 1;
};

struct LineSeries {
    Column x;
    Column y;

    NodePlot::Color color;
    double stroke_width = 1;
};

struct RibbonSeries {
    Column x;
    Column y_min;
    Column y_max;

    NodePlot::Color color;
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

struct PlotStyle {
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

struct Plot {
    std::vector<Series> series;
    std::string title;
    std::string x_label;
    std::string y_label;
    bool x_axis_log_scale;
    bool y_axis_log_scale;
    PlotStyle style;
};

enum class DataType {
    NUMBER,
    INTEGER,
    STRING,
    BOOLEAN,

    TABLE,

    COLUMN,

    SERIES,
    PLOT,

    MARGINES,
    COLOR,

    PLOT_STYLE,
};

using Data = std::variant<double, int64_t, std::string, bool, Table, Column, Series, Plot, Margins, Color, PlotStyle>;

struct NodePlotFile;
struct EvaluatedNodeGraph;

struct NodeOutputCache {
    std::map<OutputId, Data> computed_outputs;
    std::optional<std::string> error;
};

namespace InputAttribute {
struct FilePath {};
struct AutoFillsFromTable {
    InputId id;
};
} // namespace InputAttribute

struct Node {
    using InputAttribute = std::variant<InputAttribute::FilePath, InputAttribute::AutoFillsFromTable>;

    struct Input {
        InputId id;
        std::string display_name;
        std::vector<DataType> valid_data_types;
        std::optional<Data> default_value;
        std::vector<InputAttribute> attributes = {};
    };

    struct Output {
        OutputId id;
        std::string display_name;
        std::vector<DataType> valid_data_types;
    };

    NodeTypeId type_id;
    std::string display_name;

    std::function<ErrorOr<std::vector<std::pair<InputId, Input>>>(NodePlotFile*, EvaluatedNodeGraph*, NodeId)> inputs;
    std::function<ErrorOr<std::vector<std::pair<OutputId, Output>>>(NodePlotFile*, EvaluatedNodeGraph*, NodeId)> outputs;

    std::function<ErrorOr<void>(NodePlotFile*, EvaluatedNodeGraph*, NodeId, NodeOutputCache&)> evalulate;
};

struct NodeRegistry {
    static std::map<NodeTypeId, Node> node_map;

    static void init();

    static void register_node(NodeTypeId node_id, Node node);
};

struct NodeGraph {
    struct InputPin {
        NodeId node_id;
        OutputId output_id;
    };

    struct NodeStorage {
        NodeTypeId type_id;

        struct {
            float x = 0;
            float y = 0;
        } pos;

        std::map<InputId, std::variant<Data, InputPin>> input_storage;
    };

    std::map<NodeId, NodeStorage> nodes;
    NodeId next_free_node_id = 0;

    static ErrorOr<NodeGraph> from_json(nlohmann::json json);

    ErrorOr<nlohmann::json> to_json();

    ErrorOr<NodeId> create_node(NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeTypeId type, float x, float y);
};

struct NodePlotFile {
    std::filesystem::path path;
    std::map<GraphId, NodeGraph> graphs;

    static ErrorOr<NodePlotFile> create(std::filesystem::path path);

    static ErrorOr<NodePlotFile> from_json(nlohmann::json json, std::filesystem::path path);

    ErrorOr<nlohmann::json> to_json();
};

struct EvaluatedNodeGraph {
    GraphId graph_id;

    std::map<NodeId, NodeOutputCache> cache;

    ErrorOr<std::reference_wrapper<NodeGraph>> node_graph(NodePlotFile* npf) {
        auto ng = npf->graphs.find(graph_id);
        if (ng == npf->graphs.end())
            return ERR("Invalid graph id");
        return ng->second;
    }

    ErrorOr<Data> get_output_data(NodePlotFile* npf, NodeId node_id, OutputId output_id);
    ErrorOr<Data> get_input_data(NodePlotFile* npf, NodeId node_id, InputId input_id);

    template <typename T>
    ErrorOr<T> try_data_type_conversion(Data data) {
        return std::visit(overloaded{[](std::string v) -> ErrorOr<T> {
                                         if constexpr (std::is_same_v<T, std::string>)
                                             return v;
                                         if constexpr (std::is_same_v<T, std::filesystem::path>)
                                             return std::filesystem::path(v);
                                         return ERR("Invalid type, cannot convert from string. Value: '" + v + "'");
                                     },
                                     [](auto v) -> ErrorOr<T> {
                                         if constexpr (std::is_same_v<decltype(v), T>)
                                             return v;

                                         return ERR("Invalid type, unknown source type");
                                     }},
                          data);
    }

    template <typename T>
    ErrorOr<T> get_input_value(NodePlotFile* npf, NodeId node_id, InputId input_id) {
        auto data = get_input_data(npf, node_id, input_id);
        if (!data.has_value()) {
            return ERR(input_id + ": " + data.error());
        }

        auto convert = try_data_type_conversion<T>(data.value());
        if (!convert.has_value()) {
            return ERR(input_id + ": " + convert.error());
        }

        return convert.value();
    }

    template <typename... Ts>
    ErrorOr<std::variant<Ts...>> get_input_value_variant(NodePlotFile* npf, NodeId node_id, InputId input_id) {
        auto data = get_input_data(npf, node_id, input_id);
        if (!data.has_value()) {
            return ERR(input_id + ": " + data.error());
        }

        std::array<ErrorOr<std::variant<Ts...>>, sizeof...(Ts)> res;
        size_t i = 0;
        (
            [&]() {
                auto val = try_data_type_conversion<Ts>(data.value());
                if (val.has_value())
                    res[i++] = val.value();
                else
                    res[i++] = std::unexpected(val.error());
            }(),
            ...);

        for (size_t i = 0; i < sizeof...(Ts); i++)
            if (res[i].has_value())
                return res[i];

        return ERR(input_id + ": Invalid type");
    }

    ErrorOr<void> node_inputs_changed(NodePlotFile* npf, NodeId id) {
        auto& ng = TRY(node_graph(npf)).get();

        std::set<NodeId> seen = {id};
        std::stack<NodeId> to_check;
        to_check.push(id);

        while (!to_check.empty()) {
            auto id = to_check.top();
            to_check.pop();

            cache[id] = {};

            for (auto [other_id, other_storage] : ng.nodes) {
                for (auto [input_id, value] : other_storage.input_storage) {
                    if (std::holds_alternative<NodeGraph::InputPin>(value)) {
                        auto& pin = std::get<NodeGraph::InputPin>(value);
                        if (pin.node_id == id) {
                            if (seen.insert(other_id).second) {
                                to_check.push(other_id);
                            };
                        }
                    }
                }
            }
        }

        return {};
    }
};

} // namespace NodePlot
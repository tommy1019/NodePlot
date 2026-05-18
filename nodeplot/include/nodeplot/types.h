#pragma once

#include <charconv>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "error.h"
#include "utils.h"

namespace NodePlot {

using GraphId = std::string;

using NodeTypeId = std::string;

using InputId = std::string;
using OutputId = std::string;

using NodeId = ssize_t;

struct NodePlotFile;
struct EvaluatedNodeGraph;
struct NodeOutputCache;

struct MappedFile {
    char* data;
    size_t file_size;
    std::string_view full_string_view;
    ~MappedFile();
};

struct Color {
    float r, g, b, a;
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

        return std::visit(Utils::overloaded{
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

struct Pos {
    float x;
    float y;
};

struct Margins {
    float left;
    float right;
    float top;
    float bottom;
};

struct PlotStyle {
    Margins plot_margines;
    Margins internal_plot_margines;

    double x_axis_tick_mark_font_size;
    double x_axis_tick_mark_size;
    Pos x_axis_tick_mark_offset;
    double x_axis_label_font_size;
    Pos x_axis_label_offset;

    double y_axis_tick_mark_font_size;
    double y_axis_tick_mark_size;
    Pos y_axis_tick_mark_offset;
    double y_axis_label_font_size;
    Pos y_axis_label_offset;

    double title_font_size;
    Pos title_offset;
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

namespace DrawCommands {
struct Line {
    Pos start;
    Pos end;
    Color color;
    double stroke_width;
};
struct Circle {
    Pos pos;
    double r;
    Color color;
};
struct Polygon {
    std::vector<Pos> points;
    Color stroke_color;
    Color fill_color;
    double stroke_width;
};
struct Text {
    Pos pos;
    std::string text;
    enum { LEFT, MIDDLE, RIGHT } anchor;
    double font_size;
    double rotate = 0;
};
} // namespace DrawCommands

using DrawCommand = std::variant<DrawCommands::Line, DrawCommands::Circle, DrawCommands::Polygon, DrawCommands::Text>;

struct Figure {
    std::vector<DrawCommand> commands;
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

    FIGURE,

    POSITION,
    MARGINES,
    COLOR,

    PLOT_STYLE,
};

using Data = std::variant<double, int64_t, std::string, bool, Table, Column, Series, Plot, Figure, Pos, Margins, Color, PlotStyle>;

} // namespace NodePlot

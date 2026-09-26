#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace NodePlot {

using GraphId = std::string;

using NodeTypeId = std::string;
using SeriesTypeId = std::string;

using InputId = std::string;
using OutputId = std::string;

using NodeId = ssize_t;

struct InputPin {
    NodeId node_id;
    OutputId output_id;
};

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

struct TableData {
    std::map<std::string, std::vector<std::string>> columns;
    std::vector<std::string> column_names;
};
using Table = std::shared_ptr<TableData>;

struct Pos {
    float x;
    float y;
};

struct Limits {
    double x_low, x_high, y_low, y_high;
};

struct Margins {
    float left;
    float right;
    float top;
    float bottom;
};

struct PlotStyle {
    Margins plot_margins;
    Margins internal_plot_margins;

    double title_font_size;
    Pos title_offset;

    double x_axis_stroke_width;
    double x_axis_tick_mark_font_size;
    double x_axis_tick_mark_size;
    double x_axis_tick_mark_stroke_width;
    Pos x_axis_tick_mark_offset;
    double x_axis_label_font_size;
    Pos x_axis_label_offset;

    double y_axis_stroke_width;
    double y_axis_tick_mark_font_size;
    double y_axis_tick_mark_size;
    double y_axis_tick_mark_stroke_width;
    Pos y_axis_tick_mark_offset;
    double y_axis_label_font_size;
    Pos y_axis_label_offset;
};

static constexpr PlotStyle DEFAULT_PLOT_STYLE = {
    .plot_margins = NodePlot::Margins{.left = 0.16f, .right = 0.05f, .top = 0.14f, .bottom = 0.15f},
    .internal_plot_margins = NodePlot::Margins{.left = 0, .right = 0, .top = 0, .bottom = 0},

    .title_font_size = 16,
    .title_offset = {.x = 0.0f, .y = 0.1f},

    .x_axis_stroke_width = 2.0,
    .x_axis_tick_mark_font_size = 12,
    .x_axis_tick_mark_size = 0.02,
    .x_axis_tick_mark_stroke_width = 2.0,
    .x_axis_tick_mark_offset = {.x = 0.0, .y = 0.04},
    .x_axis_label_font_size = 12,
    .x_axis_label_offset = {.x = 0, .y = 0},

    .y_axis_stroke_width = 2.0,
    .y_axis_tick_mark_font_size = 12,
    .y_axis_tick_mark_size = 0.02,
    .y_axis_tick_mark_stroke_width = 2.0,
    .y_axis_tick_mark_offset = {.x = 0.01, .y = 0.015},
    .y_axis_label_font_size = 12,
    .y_axis_label_offset = {.x = 0.06f, .y = 0.0f},
};

namespace DrawCommands {
struct Line {
    std::vector<Pos> points;
    Color color;
    double stroke_width;
    std::string dash_pattern;
};
struct Circle {
    Pos pos;
    double r;
    Color color;
};
struct Rect {
    Pos a;
    Pos b;
    Color color;
    Color stroke_color = Color{0, 0, 0, 0};
    double stroke_width = 0;
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
    bool bold = false;
};
} // namespace DrawCommands

using DrawCommand = std::variant<DrawCommands::Line, DrawCommands::Circle, DrawCommands::Rect, DrawCommands::Polygon, DrawCommands::Text>;

struct Figure {
    std::vector<DrawCommand> commands;
};
struct FigureBounds {
    double x_min;
    double x_max;

    double y_min;
    double y_max;

    bool x_axis_log_scale;
    bool y_axis_log_scale;

    double x_transform_pre;
    double y_transform_pre;

    double x_scale;
    double y_scale;

    double x_transform_post;
    double y_transform_post;

    bool in_range(Pos p) const;
    bool in_range_x(double x) const;
    bool in_range_y(double y) const;

    Pos normalize(Pos p) const;
};

enum class DataType {
    NUMBER,
    NUMBER_COLUMN,
    INTEGER,
    INTEGER_COLUMN,
    STRING,
    STRING_COLUMN,
    BOOLEAN,
    BOOLEAN_COLUMN,

    TABLE,

    SERIES,

    FIGURE,

    POSITION,
    MARGINS,
    COLOR,
    COLOR_COLUMN,

    PLOT_STYLE,
};

struct GenericSeries;

using Data = std::variant<double,
                          std::vector<double>,
                          int64_t,
                          std::vector<int64_t>,
                          std::string,
                          std::vector<std::string>,
                          bool,
                          std::vector<bool>,
                          Table,
                          GenericSeries,
                          Figure,
                          Pos,
                          Margins,
                          Color,
                          std::vector<Color>,
                          PlotStyle>;

struct GenericSeries {
    SeriesTypeId type_id;
    std::map<InputId, Data> data;
};

} // namespace NodePlot

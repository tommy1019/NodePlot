#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

#include "error.h"

namespace NodePlot {

using GraphId = std::string;

using NodeTypeId = std::string;
using SeriesTypeId = std::string;

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

struct Table {
    std::map<std::string, std::vector<std::string>> columns;
    std::vector<std::string> column_names;
};

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
struct FigureBounds {
    bool x_axis_log_scale;
    bool y_axis_log_scale;

    double x_transform_pre;
    double y_transform_pre;

    double x_scale;
    double y_scale;

    double x_transform_post;
    double y_transform_post;

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
    MARGINES,
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

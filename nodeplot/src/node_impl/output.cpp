#include "error.h"
#include "nodeplot.h"

#include <format>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace NodePlot;

void register_output() {
    NodeRegistry::register_node(
        "output",
        Node{
            .type_id = "output",
            .display_name = "Output",
            .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                std::vector<std::pair<InputId, Node::Input>> res;

                res.emplace_back("filename", Node::Input{.id = "filename", .display_name = "Filename", .valid_data_types = {DataType::STRING}});

                res.emplace_back("width", Node::Input{.id = "width", .display_name = "Width", .valid_data_types = {DataType::NUMBER}, .default_value = 300.0});
                res.emplace_back("height", Node::Input{.id = "height", .display_name = "Height", .valid_data_types = {DataType::NUMBER}, .default_value = 300.0});

                res.emplace_back("grid_cols", Node::Input{.id = "grid_cols", .display_name = "Grid Cols", .valid_data_types = {DataType::INTEGER}, .default_value = 1});
                res.emplace_back("grid_rows", Node::Input{.id = "grid_rows", .display_name = "Grid Rows", .valid_data_types = {DataType::INTEGER}, .default_value = 1});

                int64_t grid_cols = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "grid_cols").value_or(0), int64_t{0}, int64_t{255});
                int64_t grid_rows = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "grid_rows").value_or(0), int64_t{0}, int64_t{255});

                for (int64_t j = 0; j < grid_rows; j++) {
                    for (int64_t i = 0; i < grid_cols; i++) {
                        std::string name = "plot_" + std::to_string(i) + "_" + std::to_string(j);
                        res.emplace_back(name, Node::Input{.id = name, .display_name = "Plot " + std::to_string(i) + " " + std::to_string(j), .valid_data_types = {DataType::PLOT}});
                    }
                }

                return res;
            },
            .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                return {
                    {"svg", Node::Output{.id = "svg", .display_name = "SVG", .valid_data_types = {DataType::STRING}}},
                };
            },
            .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                auto get_internal_svg = [](Plot graph, double width, double height, double x_offset, double y_offset) -> ErrorOr<std::string> {
                    std::pair<double, double> x_lims = {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
                    std::pair<double, double> y_lims = {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

                    auto expand_limits = [](Column::Numeric& col, std::pair<double, double>& lims) {
                        for (auto& v : col.values) {
                            lims.first = std::min(lims.first, v);
                            lims.second = std::max(lims.second, v);
                        }
                    };

                    for (auto& series : graph.series) {
                        TRY(std::visit(Utils::overloaded{
                                           [&](ScatterSeries& s) -> ErrorOr<void> {
                                               auto x = TRY(s.x.as_numeric_column());
                                               auto y = TRY(s.y.as_numeric_column());
                                               if (x.values.size() != y.values.size())
                                                   return ERR("Series x and y columns don't have the same amount of values");
                                               expand_limits(x, x_lims);
                                               expand_limits(y, y_lims);
                                               return {};
                                           },
                                           [&](LineSeries& s) -> ErrorOr<void> {
                                               auto x = TRY(s.x.as_numeric_column());
                                               auto y = TRY(s.y.as_numeric_column());
                                               if (x.values.size() != y.values.size())
                                                   return ERR("Series x and y columns don't have the same amount of values");
                                               expand_limits(x, x_lims);
                                               expand_limits(y, y_lims);
                                               return {};
                                           },
                                           [&](RibbonSeries& s) -> ErrorOr<void> {
                                               auto x = TRY(s.x.as_numeric_column());
                                               auto y_min = TRY(s.y_min.as_numeric_column());
                                               auto y_max = TRY(s.y_max.as_numeric_column());
                                               if (x.values.size() != y_min.values.size() || x.values.size() != y_max.values.size())
                                                   return ERR("Series x, y_min, and y_max columns don't have the same amount of values");
                                               expand_limits(x, x_lims);
                                               expand_limits(y_min, y_lims);
                                               expand_limits(y_max, y_lims);
                                               return {};
                                           },
                                       },
                                       series));
                    }

                    if (graph.x_axis_log_scale) {
                        x_lims.first = std::log10(x_lims.first);
                        x_lims.second = std::log10(x_lims.second);
                    }

                    if (graph.y_axis_log_scale) {
                        y_lims.first = std::log10(y_lims.first);
                        y_lims.second = std::log10(y_lims.second);
                    }

                    double x_range = x_lims.second - x_lims.first;
                    double y_range = y_lims.second - y_lims.first;

                    auto tick_interval = [](double range, int32_t max_count) {
                        double x = std::pow(10.0, std::floor(std::log10(range)));
                        if (range / x <= max_count)
                            return x;
                        if (range / (x * 2.0) <= max_count)
                            return x * 2.0;
                        if (range / (x * 5.0) <= max_count)
                            return x * 5.0;
                        return x * 10.0;
                    };

                    double x_tick_interval = tick_interval(x_range, 5);
                    double x_tick_start = std::floor(x_lims.first / x_tick_interval) * x_tick_interval;
                    double x_tick_end = std::ceil(x_lims.second / x_tick_interval) * x_tick_interval;

                    double y_tick_interval = tick_interval(y_range, 5);
                    double y_tick_start = std::floor(y_lims.first / y_tick_interval) * y_tick_interval;
                    double y_tick_end = std::ceil(y_lims.second / y_tick_interval) * y_tick_interval;

                    x_lims = {x_tick_start, x_tick_end};
                    y_lims = {y_tick_start, y_tick_end};
                    x_range = x_lims.second - x_lims.first;
                    y_range = y_lims.second - y_lims.first;

                    std::stringstream ss;

                    auto line = [&](double x1, double y1, double x2, double y2, std::string stroke = "black", std::string stroke_opacity = "100%", float stroke_width = 2) {
                        ss << "<line x1=\"" << (x1 + x_offset) << "\" y1=\"" << (y1 + y_offset) << "\" x2=\"" << (x2 + x_offset) << "\" y2=\"" << (y2 + y_offset) << "\" stroke=\"" << stroke
                           << "\" stroke-opacity=\"" << stroke_opacity << "\" stroke-width=\"" << stroke_width << "\" stroke-linecap=\"round\" />\n";
                    };

                    auto text = [&](double x, double y, std::string text, double font_size = 12, std::string text_anchor = "middle") {
                        ss << "<text x=\"" << (x + x_offset) << "\" y=\"" << (y + y_offset) << "\" font-family=\"sans-serif\" text-anchor=\"" << text_anchor << "\" font-size=\"" << font_size << "\">";
                        ss << text << "";
                        ss << "</text>\n";
                    };

                    auto vtext = [&](double x, double y, std::string text, double font_size = 12, std::string text_anchor = "middle") {
                        ss << "<g transform=\"translate(" << (x + x_offset) << ", " << (y + y_offset) << ")\">\n";
                        ss << "<text font-family=\"sans-serif\" text-anchor=\"" << text_anchor << "\" font-size=\"" << font_size << "\" transform=\"rotate(-90)\" >";
                        ss << text << "";
                        ss << "</text> </g>\n";
                    };

                    auto circle = [&](double x, double y, double r, std::string fill = "red", std::string opacity = "100%") {
                        ss << "<circle cx=\"" << (x + x_offset) << "\" cy=\"" << (y + y_offset) << "\" r=\"" << r << "\" fill=\"" << fill << "\" fill-opacity=\"" << opacity << "\" />\n";
                    };

                    // ss << "<rect x=\"" << x_offset << "\" y=\"" << y_offset << "\" width=\"" << width << "\" height=\"" << height << "\" stroke=\"black\" fill=\"rgb(1, 0, 1, 0.5)\" />\n";

                    auto& sty = graph.style;

                    struct {
                        float x, y, w, h;
                    } plot = {
                        .x = sty.plot_margines.left,
                        .y = (float)height - sty.plot_margines.bottom,
                        .w = (float)width - sty.plot_margines.left - sty.plot_margines.right,
                        .h = (float)height - sty.plot_margines.top - sty.plot_margines.bottom,
                    };

                    double plot_range_x = plot.w - sty.internal_plot_margines.left - sty.internal_plot_margines.right;
                    double plot_range_y = plot.h - sty.internal_plot_margines.bottom - sty.internal_plot_margines.top;

                    auto normalize_coords = [&](std::pair<double, double> p) -> std::pair<double, double> {
                        double x = p.first;
                        double y = p.second;

                        if (graph.x_axis_log_scale)
                            x = std::log10(x);
                        if (graph.y_axis_log_scale)
                            y = std::log10(y);

                        x = (x - x_lims.first) / x_range * plot_range_x + plot.x + sty.internal_plot_margines.left;
                        y = plot.y - (y - y_lims.first) / y_range * plot_range_y - sty.internal_plot_margines.bottom;

                        return {x, y};
                    };

                    for (auto& series : graph.series) {
                        std::visit(Utils::overloaded{
                                       [&](ScatterSeries& s) -> ErrorOr<void> {
                                           auto x_col = TRY(s.x.as_numeric_column());
                                           auto y_col = TRY(s.y.as_numeric_column());
                                           for (size_t i = 0; i < x_col.values.size(); i++) {
                                               auto [x, y] = normalize_coords({x_col.values[i], y_col.values[i]});
                                               circle(x,
                                                      y,
                                                      s.point_size,
                                                      "rgb(" + std::to_string(s.color.r * 100) + "%, " + std::to_string(s.color.g * 100) + "%, " + std::to_string(s.color.b * 100) + "%)",
                                                      std::to_string(s.color.a * 100) + "%");
                                           }
                                           return {};
                                       },
                                       [&](LineSeries& s) -> ErrorOr<void> {
                                           auto x_col = TRY(s.x.as_numeric_column());
                                           auto y_col = TRY(s.y.as_numeric_column());
                                           for (size_t i = 1; i < x_col.values.size(); i++) {
                                               auto [x1, y1] = normalize_coords({x_col.values[i - 1], y_col.values[i - 1]});
                                               auto [x2, y2] = normalize_coords({x_col.values[i], y_col.values[i]});
                                               line(x1,
                                                    y1,
                                                    x2,
                                                    y2,
                                                    "rgb(" + std::to_string(s.color.r * 100) + "%, " + std::to_string(s.color.g * 100) + "%, " + std::to_string(s.color.b * 100) + "%)",
                                                    std::to_string(s.color.a * 100) + "%",
                                                    s.stroke_width);
                                           }
                                           return {};
                                       },
                                       [&](RibbonSeries& s) -> ErrorOr<void> {
                                           auto x_col = TRY(s.x.as_numeric_column());
                                           auto y_min_col = TRY(s.y_min.as_numeric_column());
                                           auto y_max_col = TRY(s.y_max.as_numeric_column());

                                           ss << "<polygon points=\"";

                                           for (size_t i = 0; i < x_col.values.size(); i++) {
                                               auto [x, y] = normalize_coords({x_col.values[i], y_min_col.values[i]});
                                               if (i != 0)
                                                   ss << ",";
                                               ss << (x + x_offset) << "," << (y + y_offset);
                                           }

                                           for (size_t i = x_col.values.size(); i > 0; i--) {
                                               auto [x, y] = normalize_coords({x_col.values[i - 1], y_max_col.values[i - 1]});
                                               ss << ",";
                                               ss << (x + x_offset) << "," << (y + y_offset);
                                           }

                                           if (x_col.values.size() > 0) {
                                               auto [x, y] = normalize_coords({x_col.values[0], y_min_col.values[0]});
                                               ss << "," << (x + x_offset) << "," << (y + y_offset);
                                           }

                                           ss << "\" fill-opacity=\"" << (s.color.a * 100) << "%\" fill=\"rgb(" << (s.color.r * 100) << "%, " << (s.color.g * 100) << "%, " << (s.color.b * 100)
                                              << "%)\" stroke-opacity=\"" << (s.color.a * 100) << "%\" stroke=\"rgb(" << (s.color.r * 100) << "%, " << (s.color.g * 100) << "%, " << (s.color.b * 100)
                                              << "%)\"  />\n";
                                           return {};
                                       },
                                   },
                                   series);
                    }

                    line(plot.x, plot.y, plot.x + plot.w, plot.y);
                    line(plot.x, plot.y, plot.x, plot.y - plot.h);

                    auto format_tick_number = [](double v) {
                        bool e_format = false;
                        if (std::log10(v) >= 4) {
                            e_format = true;
                            v = std::log10(v);
                        }

                        auto s = std::format("{:.2f}", v);
                        auto remove_trailing_zeros = [](std::string s) {
                            if (!s.contains('.'))
                                return s;

                            for (ssize_t i = s.size() - 1; i >= 0; i--) {
                                if (s[i] != '0') {
                                    if (s[i] == '.')
                                        return s.substr(0, i);
                                    return s.substr(0, i + 1);
                                }
                            }
                            return s;
                        };

                        s = remove_trailing_zeros(s);

                        return e_format ? ("1e" + s) : s;
                    };

                    auto x_tick = [&](double x) {
                        float x_pos = normalize_coords({x, 0}).first;
                        line(x_pos, plot.y, x_pos, plot.y + sty.x_axis_tick_mark_size);
                        text(x_pos, plot.y + sty.x_axis_tick_mark_font_size + sty.x_axis_tick_mark_size, format_tick_number(x), sty.x_axis_tick_mark_font_size, "middle");
                    };

                    auto y_tick = [&](double y) {
                        float y_pos = normalize_coords({0, y}).second;
                        line(plot.x, y_pos, plot.x - sty.y_axis_tick_mark_size, y_pos);
                        text(plot.x - sty.y_axis_tick_mark_size - 2, y_pos + sty.y_axis_tick_mark_font_size / 3, format_tick_number(y), sty.y_axis_tick_mark_font_size, "end");
                    };

                    for (double x = x_tick_start; x <= x_lims.second; x += x_tick_interval) {
                        x_tick(graph.x_axis_log_scale ? std::pow(10, x) : x);
                    }

                    for (double y = y_tick_start; y <= y_lims.second; y += y_tick_interval) {
                        y_tick(graph.y_axis_log_scale ? std::pow(10, y) : y);
                    }

                    text(width / 2, height - 2, graph.x_label, sty.title_font_size, "middle");
                    vtext(sty.y_axis_label_font_size + 2, height / 2, graph.y_label, sty.title_font_size, "middle");

                    text(width / 2, sty.title_font_size, graph.title, sty.title_font_size, "middle");

                    return ss.str();
                };

                double width = TRY(eng->get_input_value<double>(npf, node_id, "width"));
                double height = TRY(eng->get_input_value<double>(npf, node_id, "height"));

                int64_t grid_cols = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "grid_cols").value_or(0), int64_t{0}, int64_t{255});
                int64_t grid_rows = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "grid_rows").value_or(0), int64_t{0}, int64_t{255});

                std::stringstream ss;
                ss << "<svg width=\"" << width << "\" height=\"" << height << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

                double width_per_col = width / grid_cols;
                double height_per_row = height / grid_rows;

                for (size_t r = 0; r < grid_rows; r++) {
                    for (size_t c = 0; c < grid_cols; c++) {
                        ss << TRY(get_internal_svg(TRY(eng->get_input_value<Plot>(npf, node_id, "plot_" + std::to_string(c) + "_" + std::to_string(r))),
                                                   width_per_col,
                                                   height_per_row,
                                                   c * width_per_col,
                                                   r * height_per_row));
                    }
                }

                ss << "</svg>\n";

                cache.computed_outputs["svg"] = ss.str();

                return {};
            },
        });
}

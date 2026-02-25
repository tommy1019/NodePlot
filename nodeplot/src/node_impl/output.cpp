#include "error.h"
#include "nodeplot.h"

#include <format>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const OutputNode& node, OutputId id) {
    return ERR("Trying to get output from an output node");
}

ErrorOr<std::string> OutputNode::get_svg(EvaluatedNodeGraph* lng) {

    std::vector<Graph> graphs;
    for (auto& i_graph : i_graphs)
        graphs.push_back(TRY(lng->get_input(i_graph)));

    auto width = TRY(lng->get_input(i_width));
    auto height = TRY(lng->get_input(i_height));

    auto grid_cols = TRY(lng->get_input(i_grid_cols));
    auto grid_rows = TRY(lng->get_input(i_grid_rows));

    auto get_internal_svg = [](Graph graph, double width, double height, double x_offset, double y_offset) -> ErrorOr<std::string> {
        std::pair<double, double> x_lims = {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
        std::pair<double, double> y_lims = {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

        auto expand_limits = [](Column& col, std::pair<double, double>& lims) {
            col.ensure_numeric();
            for (auto& v : *col.numeric_values) {
                lims.first = std::min(lims.first, v);
                lims.second = std::max(lims.second, v);
            }
        };

        for (auto& series : graph.series) {
            TRY(std::visit(overloaded{
                               [&](ScatterSeries& s) -> ErrorOr<void> {
                                   if (s.x.values.size() != s.y.values.size())
                                       return ERR("Series x and y columns don't have the same amount of values");
                                   expand_limits(s.x, x_lims);
                                   expand_limits(s.y, y_lims);
                                   return {};
                               },
                               [&](LineSeries& s) -> ErrorOr<void> {
                                   if (s.x.values.size() != s.y.values.size())
                                       return ERR("Series x and y columns don't have the same amount of values");
                                   expand_limits(s.x, x_lims);
                                   expand_limits(s.y, y_lims);
                                   return {};
                               },
                               [&](RibbonSeries& s) -> ErrorOr<void> {
                                   if (s.x.values.size() != s.y_min.values.size() || s.x.values.size() != s.y_max.values.size())
                                       return ERR("Series x, y_min, and y_max columns don't have the same amount of values");
                                   expand_limits(s.x, x_lims);
                                   expand_limits(s.y_min, y_lims);
                                   expand_limits(s.y_max, y_lims);
                                   return {};
                               },
                           },
                           series));
        }

        if (graph.x_log) {
            x_lims.first = std::max(0.0, std::log10(x_lims.first));
            x_lims.second = std::log10(x_lims.second);
        }

        if (graph.y_log) {
            y_lims.first = std::max(0.0, std::log10(y_lims.first));
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

            if (graph.x_log)
                x = std::log10(x);
            if (graph.y_log)
                y = std::log10(y);

            x = (x - x_lims.first) / x_range * plot_range_x + plot.x + sty.internal_plot_margines.left;
            y = plot.y - (y - y_lims.first) / y_range * plot_range_y - sty.internal_plot_margines.bottom;

            return {x, y};
        };

        for (auto& series : graph.series) {
            std::visit(overloaded{
                           [&](ScatterSeries& s) {
                               for (size_t i = 0; i < s.x.values.size(); i++) {
                                   auto [x, y] = normalize_coords({s.x.numeric_values.value()[i], s.y.numeric_values.value()[i]});
                                   circle(x,
                                          y,
                                          s.point_size,
                                          "rgb(" + std::to_string(s.color.r * 100) + "%, " + std::to_string(s.color.g * 100) + "%, " + std::to_string(s.color.b * 100) + "%)",
                                          std::to_string(s.color.a * 100) + "%");
                               }
                           },
                           [&](LineSeries& s) {
                               for (size_t i = 1; i < s.x.values.size(); i++) {
                                   auto [x1, y1] = normalize_coords({s.x.numeric_values.value()[i - 1], s.y.numeric_values.value()[i - 1]});
                                   auto [x2, y2] = normalize_coords({s.x.numeric_values.value()[i], s.y.numeric_values.value()[i]});
                                   line(x1,
                                        y1,
                                        x2,
                                        y2,
                                        "rgb(" + std::to_string(s.color.r * 100) + "%, " + std::to_string(s.color.g * 100) + "%, " + std::to_string(s.color.b * 100) + "%)",
                                        std::to_string(s.color.a * 100) + "%",
                                        s.stroke_width);
                               }
                           },
                           [&](RibbonSeries& s) {
                               ss << "<polygon points=\"";

                               for (size_t i = 0; i < s.x.values.size(); i++) {
                                   auto [x, y] = normalize_coords({s.x.numeric_values.value()[i], s.y_min.numeric_values.value()[i]});
                                   if (i != 0)
                                       ss << ",";
                                   ss << (x + x_offset) << "," << (y + y_offset);
                               }

                               for (size_t i = s.x.values.size(); i > 0; i--) {
                                   auto [x, y] = normalize_coords({s.x.numeric_values.value()[i - 1], s.y_max.numeric_values.value()[i - 1]});
                                   ss << ",";
                                   ss << (x + x_offset) << "," << (y + y_offset);
                               }

                               if (s.x.values.size() > 0) {
                                   auto [x, y] = normalize_coords({s.x.numeric_values.value()[0], s.y_min.numeric_values.value()[0]});
                                   ss << "," << (x + x_offset) << "," << (y + y_offset);
                               }

                               ss << "\" fill-opacity=\"" << (s.color.a * 100) << "%\" fill=\"rgb(" << (s.color.r * 100) << "%, " << (s.color.g * 100) << "%, " << (s.color.b * 100)
                                  << "%)\" stroke-opacity=\"" << (s.color.a * 100) << "%\" stroke=\"rgb(" << (s.color.r * 100) << "%, " << (s.color.g * 100) << "%, " << (s.color.b * 100)
                                  << "%)\"  />\n";
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
            x_tick(graph.x_log ? std::pow(10, x) : x);
        }

        for (double y = y_tick_start; y <= y_lims.second; y += y_tick_interval) {
            y_tick(graph.y_log ? std::pow(10, y) : y);
        }

        text(width / 2, height - 2, graph.x_lab, sty.title_font_size, "middle");
        vtext(sty.y_axis_label_font_size + 2, height / 2, graph.y_lab, sty.title_font_size, "middle");

        text(width / 2, sty.title_font_size, graph.title, sty.title_font_size, "middle");

        return ss.str();
    };

    std::stringstream ss;
    ss << "<svg width=\"" << width << "\" height=\"" << height << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

    double width_per_col = width / grid_cols;
    double height_per_row = height / grid_rows;

    for (size_t r = 0; r < grid_rows; r++) {
        for (size_t c = 0; c < grid_cols; c++) {
            ss << TRY(get_internal_svg(graphs[c + r * grid_cols], width_per_col, height_per_row, c * width_per_col, r * height_per_row));
        }
    }

    ss << "</svg>\n";

    return ss.str();
}
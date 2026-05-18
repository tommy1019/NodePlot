#include "error.h"
#include "nodeplot.h"
#include "types.h"

#include <format>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using namespace NodePlot;

void register_figure() {
    NodeRegistry::register_node("plot_figure",
                                Node{
                                    .type_id = "plot_figure",
                                    .display_name = "Plot Figure",
                                    .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                                        std::vector<std::pair<InputId, Node::Input>> res;

                                        res.emplace_back("num_series", Node::Input{.id = "num_series", .display_name = "Number of Series", .valid_data_types = {DataType::INTEGER}});

                                        res.emplace_back("title", Node::Input{.id = "title", .display_name = "Title", .valid_data_types = {DataType::STRING}});
                                        res.emplace_back("x_label", Node::Input{.id = "x_label", .display_name = "X Label", .valid_data_types = {DataType::STRING}});
                                        res.emplace_back("y_label", Node::Input{.id = "y_label", .display_name = "Y Label", .valid_data_types = {DataType::STRING}});
                                        res.emplace_back("x_axis_log_scale", Node::Input{.id = "x_axis_tick_mark_size", .display_name = "X Axis Log Scale", .valid_data_types = {DataType::BOOLEAN}});
                                        res.emplace_back("y_axis_log_scale", Node::Input{.id = "y_axis_log_scale", .display_name = "X Axis Log Scale", .valid_data_types = {DataType::BOOLEAN}});
                                        res.emplace_back("style", Node::Input{.id = "style", .display_name = "Style", .valid_data_types = {DataType::PLOT_STYLE}});

                                        int64_t num_series = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "num_series").value_or(0), int64_t{0}, int64_t{255});
                                        for (int64_t i = 0; i < num_series; i++) {
                                            std::string name = "series_" + std::to_string(i);
                                            res.emplace_back(name, Node::Input{.id = name, .display_name = "Series " + std::to_string(i), .valid_data_types = {DataType::SERIES}});
                                        }

                                        return res;
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"figure", Node::Output{.id = "figure", .display_name = "Figure", .valid_data_types = {DataType::FIGURE}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        std::string title = TRY(eng->get_input_value<std::string>(npf, node_id, "title"));

                                        std::string x_label = TRY(eng->get_input_value<std::string>(npf, node_id, "x_label"));
                                        std::string y_label = TRY(eng->get_input_value<std::string>(npf, node_id, "y_label"));

                                        bool x_axis_log_scale = TRY(eng->get_input_value<bool>(npf, node_id, "x_axis_log_scale"));
                                        bool y_axis_log_scale = TRY(eng->get_input_value<bool>(npf, node_id, "y_axis_log_scale"));

                                        PlotStyle style = TRY(eng->get_input_value<PlotStyle>(npf, node_id, "style"));

                                        int64_t num_series = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "num_series").value_or(0), int64_t{0}, int64_t{255});

                                        std::vector<Series> all_series;
                                        all_series.reserve(num_series);

                                        for (int64_t i = 0; i < num_series; i++) {
                                            all_series.push_back(TRY(eng->get_input_value<Series>(npf, node_id, "series_" + std::to_string(i))));
                                        }

                                        Figure res;

                                        std::pair<double, double> x_lims = {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
                                        std::pair<double, double> y_lims = {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

                                        auto expand_limits = [](Column::Numeric& col, std::pair<double, double>& lims) {
                                            for (auto& v : col.values) {
                                                lims.first = std::min(lims.first, v);
                                                lims.second = std::max(lims.second, v);
                                            }
                                        };

                                        for (auto& series : all_series) {
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

                                        if (x_axis_log_scale) {
                                            x_lims.first = std::log10(x_lims.first);
                                            x_lims.second = std::log10(x_lims.second);
                                        }
                                        if (y_axis_log_scale) {
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

                                        double x_tick_interval = tick_interval(x_range, 3);
                                        double x_tick_start = std::floor(x_lims.first / x_tick_interval) * x_tick_interval;
                                        double x_tick_end = std::ceil(x_lims.second / x_tick_interval) * x_tick_interval;

                                        double y_tick_interval = tick_interval(y_range, 3);
                                        double y_tick_start = std::floor(y_lims.first / y_tick_interval) * y_tick_interval;
                                        double y_tick_end = std::ceil(y_lims.second / y_tick_interval) * y_tick_interval;

                                        x_lims = {x_tick_start, x_tick_end};
                                        y_lims = {y_tick_start, y_tick_end};
                                        x_range = x_lims.second - x_lims.first;
                                        y_range = y_lims.second - y_lims.first;

                                        struct {
                                            float x, y, w, h;
                                        } plot = {
                                            .x = style.plot_margines.left,
                                            .y = (float)1 - style.plot_margines.bottom,
                                            .w = (float)1 - style.plot_margines.left - style.plot_margines.right,
                                            .h = (float)1 - style.plot_margines.top - style.plot_margines.bottom,
                                        };

                                        double plot_range_x = plot.w - style.internal_plot_margines.left - style.internal_plot_margines.right;
                                        double plot_range_y = plot.h - style.internal_plot_margines.bottom - style.internal_plot_margines.top;

                                        auto normalize_coords = [&](std::pair<double, double> p) -> std::pair<double, double> {
                                            double x = p.first;
                                            double y = p.second;

                                            if (x_axis_log_scale)
                                                x = std::log10(x);
                                            if (y_axis_log_scale)
                                                y = std::log10(y);

                                            x = (x - x_lims.first) / x_range * plot_range_x + plot.x + style.internal_plot_margines.left;
                                            y = plot.y - (y - y_lims.first) / y_range * plot_range_y - style.internal_plot_margines.bottom;

                                            return {x, y};
                                        };

                                        for (auto& series : all_series) {
                                            MAYBE(std::visit(Utils::overloaded{
                                                                 [&](ScatterSeries& s) -> ErrorOr<void> {
                                                                     auto x_col = TRY(s.x.as_numeric_column());
                                                                     auto y_col = TRY(s.y.as_numeric_column());
                                                                     for (size_t i = 0; i < x_col.values.size(); i++) {
                                                                         auto [x, y] = normalize_coords({x_col.values[i], y_col.values[i]});
                                                                         res.commands.push_back(DrawCommands::Circle{
                                                                             .pos = Pos{(float)x, (float)y},
                                                                             .r = s.point_size,
                                                                             .color = s.color,
                                                                         });
                                                                     }
                                                                     return {};
                                                                 },
                                                                 [&](LineSeries& s) -> ErrorOr<void> {
                                                                     auto x_col = TRY(s.x.as_numeric_column());
                                                                     auto y_col = TRY(s.y.as_numeric_column());
                                                                     for (size_t i = 1; i < x_col.values.size(); i++) {
                                                                         auto [x1, y1] = normalize_coords({x_col.values[i - 1], y_col.values[i - 1]});
                                                                         auto [x2, y2] = normalize_coords({x_col.values[i], y_col.values[i]});
                                                                         res.commands.push_back(DrawCommands::Line{
                                                                             .start = Pos{(float)x1, (float)y1},
                                                                             .end = Pos{(float)x2, (float)y2},
                                                                             .color = s.color,
                                                                             .stroke_width = s.stroke_width,
                                                                         });
                                                                     }
                                                                     return {};
                                                                 },
                                                                 [&](RibbonSeries& s) -> ErrorOr<void> {
                                                                     auto x_col = TRY(s.x.as_numeric_column());
                                                                     auto y_min_col = TRY(s.y_min.as_numeric_column());
                                                                     auto y_max_col = TRY(s.y_max.as_numeric_column());

                                                                     std::vector<Pos> points;
                                                                     for (size_t i = 0; i < x_col.values.size(); i++) {
                                                                         auto [x, y] = normalize_coords({x_col.values[i], y_min_col.values[i]});
                                                                         points.push_back({(float)x, (float)y});
                                                                     }
                                                                     for (size_t i = x_col.values.size(); i > 0; i--) {
                                                                         auto [x, y] = normalize_coords({x_col.values[i - 1], y_max_col.values[i - 1]});
                                                                         points.push_back({(float)x, (float)y});
                                                                     }
                                                                     if (x_col.values.size() > 0) {
                                                                         auto [x, y] = normalize_coords({x_col.values[0], y_min_col.values[0]});
                                                                         points.push_back({(float)x, (float)y});
                                                                     }

                                                                     res.commands.push_back(DrawCommands::Polygon{
                                                                         .points = points,
                                                                         .stroke_color = s.color,
                                                                         .fill_color = s.color,
                                                                         .stroke_width = 0,
                                                                     });

                                                                     return {};
                                                                 },
                                                             },
                                                             series));
                                        }

                                        res.commands.push_back(DrawCommands::Line{
                                            .start = Pos{plot.x, plot.y},
                                            .end = Pos{plot.x + plot.w, plot.y},
                                            .color = Color{0, 0, 0, 1},
                                            .stroke_width = 2,
                                        });
                                        res.commands.push_back(DrawCommands::Line{
                                            .start = Pos{plot.x, plot.y},
                                            .end = Pos{plot.x, plot.y - plot.h},
                                            .color = Color{0, 0, 0, 1},
                                            .stroke_width = 2,
                                        });

                                        auto format_tick_number = [](double v) {
                                            bool e_format = false;
                                            if (std::log10(v) >= 5) {
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

                                            res.commands.push_back(DrawCommands::Line{
                                                .start = Pos{x_pos, (float)(plot.y + style.x_axis_tick_mark_size)},
                                                .end = Pos{x_pos, (float)plot.y},
                                                .color = Color{0, 0, 0, 1},
                                                .stroke_width = 2,
                                            });

                                            res.commands.push_back(DrawCommands::Text{
                                                .pos = Pos{(float)(x_pos + style.x_axis_tick_mark_offset.x), (float)(plot.y + style.x_axis_tick_mark_size + style.x_axis_tick_mark_offset.y)},
                                                .text = format_tick_number(x),
                                                .anchor = NodePlot::DrawCommands::Text::MIDDLE,
                                                .font_size = style.x_axis_tick_mark_font_size,
                                            });
                                        };
                                        auto y_tick = [&](double y) {
                                            float y_pos = normalize_coords({0, y}).second;

                                            res.commands.push_back(DrawCommands::Line{
                                                .start = Pos{plot.x, y_pos},
                                                .end = Pos{(float)(plot.x - style.y_axis_tick_mark_size), (float)y_pos},
                                                .color = Color{0, 0, 0, 1},
                                                .stroke_width = 2,
                                            });

                                            res.commands.push_back(DrawCommands::Text{
                                                .pos = Pos{(float)(plot.x - style.y_axis_tick_mark_size + style.y_axis_tick_mark_offset.x), (float)(y_pos + style.y_axis_tick_mark_offset.y)},
                                                .text = format_tick_number(y),
                                                .anchor = NodePlot::DrawCommands::Text::RIGHT,
                                                .font_size = style.x_axis_tick_mark_font_size,
                                            });
                                        };

                                        for (double x = x_tick_start; x <= x_lims.second; x += x_tick_interval) {
                                            x_tick(x_axis_log_scale ? std::pow(10, x) : x);
                                        }
                                        for (double y = y_tick_start; y <= y_lims.second; y += y_tick_interval) {
                                            y_tick(y_axis_log_scale ? std::pow(10, y) : y);
                                        }

                                        res.commands.push_back(DrawCommands::Text{
                                            .pos = Pos{0.5f + style.x_axis_label_offset.x, 1.0f + style.x_axis_label_offset.y},
                                            .text = x_label,
                                            .anchor = NodePlot::DrawCommands::Text::MIDDLE,
                                            .font_size = style.title_font_size,
                                        });
                                        res.commands.push_back(DrawCommands::Text{
                                            .pos = Pos{0.0f + style.y_axis_label_offset.x, 0.5f + style.y_axis_label_offset.y},
                                            .text = y_label,
                                            .anchor = NodePlot::DrawCommands::Text::MIDDLE,
                                            .font_size = style.title_font_size,
                                            .rotate = -90,
                                        });

                                        res.commands.push_back(DrawCommands::Text{
                                            .pos = Pos{0.5f + style.title_offset.x, 0.0f + style.title_offset.y},
                                            .text = title,
                                            .anchor = NodePlot::DrawCommands::Text::MIDDLE,
                                            .font_size = style.title_font_size,
                                        });

                                        cache.computed_outputs["figure"] = res;

                                        return {};
                                    },
                                });
}

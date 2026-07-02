#include "error.h"
#include "node_registry.h"
#include "nodeplot.h"
#include "types.h"
#include "utils.h"
#include <type_traits>
#include <variant>
#include <vector>

using namespace NodePlot;

void register_series_create() {
    NodeRegistry::register_series(
        "scatter",
        [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
            return {
                {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                {"y", Node::Input{.id = "y", .display_name = "Y", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR, DataType::COLOR_COLUMN}, .default_value = Color{.r = 1, .g = 0, .b = 0, .a = 1}}},
                {"point_size", Node::Input{.id = "point_size", .display_name = "Point Size", .valid_data_types = {DataType::NUMBER, NodePlot::DataType::NUMBER_COLUMN}, .default_value = 3.0}},
            };
        },
        {
            .display_name = "Scatter",
            .get_limits = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s) -> ErrorOr<Limits> {
                auto x = Utils::find_limits(TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "x", "Missing")))));
                auto y = Utils::find_limits(TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "y", "Missing")))));
                return Limits{
                    .x_low = x.first,
                    .x_high = x.second,
                    .y_low = y.first,
                    .y_high = y.second,
                };
            },
            .evalulate = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s, Figure& fig, const FigureBounds& bounds) -> ErrorOr<void> {
                auto x_col = TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "x", "Missing"))));
                auto y_col = TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "y", "Missing"))));
                auto color = TRY(eng->try_data_type_conversion_variant<Color, std::vector<Color>>(TRY(Utils::try_find(s.data, "color", "Missing"))));
                auto point_size = TRY(eng->try_data_type_conversion_variant<double, std::vector<double>>(TRY(Utils::try_find(s.data, "point_size", "Missing"))));

                if (x_col.size() != y_col.size())
                    return ERR("Columns are not of the same size");
                if (std::holds_alternative<std::vector<Color>>(color))
                    if (x_col.size() != std::get<std::vector<Color>>(color).size())
                        return ERR("Columns are not of the same size");
                if (std::holds_alternative<std::vector<double>>(point_size))
                    if (x_col.size() != std::get<std::vector<double>>(point_size).size())
                        return ERR("Columns are not of the same size");

                std::visit(
                    [&](auto point_size, auto color) {
                        for (size_t i = 0; i < x_col.size(); i++) {
                            auto [x, y] = bounds.normalize({(float)x_col[i], (float)y_col[i]});

                            Color color_val;
                            if constexpr (std::is_same_v<decltype(color), Color>)
                                color_val = color;
                            else
                                color_val = color[i];

                            double point_size_val;
                            if constexpr (std::is_same_v<decltype(point_size), double>)
                                point_size_val = point_size;
                            else
                                point_size_val = point_size[i];

                            fig.commands.push_back(DrawCommands::Circle{
                                .pos = Pos{(float)x, (float)y},
                                .r = point_size_val,
                                .color = color_val,
                            });
                        }
                    },
                    point_size,
                    color);

                return {};
            },
        });

    NodeRegistry::register_series("line",
                                  [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                      return {
                                          {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                          {"y", Node::Input{.id = "y", .display_name = "Y", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                                          {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR}, .default_value = Color{.r = 1, .g = 0, .b = 0, .a = 1}}},
                                          {"stroke_width", Node::Input{.id = "stroke_width", .display_name = "Stroke Width", .valid_data_types = {DataType::NUMBER}, .default_value = 3.0}},
                                      };
                                  },
                                  {
                                      .display_name = "Line",
                                      .get_limits = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s) -> ErrorOr<Limits> {
                                          auto x = Utils::find_limits(TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "x", "Missing")))));
                                          auto y = Utils::find_limits(TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "y", "Missing")))));
                                          return Limits{
                                              .x_low = x.first,
                                              .x_high = x.second,
                                              .y_low = y.first,
                                              .y_high = y.second,
                                          };
                                      },
                                      .evalulate = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s, Figure& fig, const FigureBounds& bounds) -> ErrorOr<void> {
                                          auto x_col = TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "x", "Missing"))));
                                          auto y_col = TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "y", "Missing"))));
                                          auto color = TRY(eng->try_data_type_conversion<Color>(TRY(Utils::try_find(s.data, "color", "Missing"))));
                                          auto stroke_width = TRY(eng->try_data_type_conversion<double>(TRY(Utils::try_find(s.data, "stroke_width", "Missing"))));

                                          if (x_col.size() != y_col.size())
                                              return ERR("Columns are not of the same size");

                                          for (size_t i = 1; i < x_col.size(); i++) {
                                              auto [x1, y1] = bounds.normalize({(float)x_col[i - 1], (float)y_col[i - 1]});
                                              auto [x2, y2] = bounds.normalize({(float)x_col[i], (float)y_col[i]});
                                              fig.commands.push_back(DrawCommands::Line{
                                                  .start = Pos{(float)x1, (float)y1},
                                                  .end = Pos{(float)x2, (float)y2},
                                                  .color = color,
                                                  .stroke_width = stroke_width,
                                              });
                                          }
                                          return {};
                                      },
                                  });

    NodeRegistry::register_series(
        "ribbon",
        [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
            return {
                {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                {"y_min", Node::Input{.id = "y_min", .display_name = "Y Min", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                {"y_max", Node::Input{.id = "y_max", .display_name = "Y Max", .valid_data_types = {DataType::NUMBER_COLUMN}}},
                {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR}, .default_value = Color{.r = 1, .g = 0, .b = 0, .a = 0.2}}},
            };
        },
        {
            .display_name = "Ribbon",
            .get_limits = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s) -> ErrorOr<Limits> {
                auto x = Utils::find_limits(TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "x", "Missing")))));
                auto y_min = Utils::find_limits(TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "y_min", "Missing")))));
                auto y_max = Utils::find_limits(TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "y_max", "Missing")))));
                return Limits{
                    .x_low = x.first,
                    .x_high = x.second,
                    .y_low = std::min(y_min.first, y_max.first),
                    .y_high = std::max(y_min.second, y_max.second),
                };
            },
            .evalulate = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s, Figure& fig, const FigureBounds& bounds) -> ErrorOr<void> {
                auto x_col = TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "x", "Missing"))));
                auto y_min_col = TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "y_min", "Missing"))));
                auto y_max_col = TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "y_max", "Missing"))));
                auto color = TRY(eng->try_data_type_conversion<Color>(TRY(Utils::try_find(s.data, "color", "Missing"))));

                if (x_col.size() != y_min_col.size() && x_col.size() != y_max_col.size())
                    return ERR("Columns are not of the same size");

                std::vector<Pos> points;
                for (size_t i = 0; i < x_col.size(); i++) {
                    auto [x, y] = bounds.normalize({(float)x_col[i], (float)y_min_col[i]});
                    points.push_back({(float)x, (float)y});
                }
                for (size_t i = x_col.size(); i > 0; i--) {
                    auto [x, y] = bounds.normalize({(float)x_col[i - 1], (float)y_max_col[i - 1]});
                    points.push_back({(float)x, (float)y});
                }
                if (x_col.size() > 0) {
                    auto [x, y] = bounds.normalize({(float)x_col[0], (float)y_min_col[0]});
                    points.push_back({(float)x, (float)y});
                }

                fig.commands.push_back(DrawCommands::Polygon{
                    .points = points,
                    .stroke_color = color,
                    .fill_color = color,
                    .stroke_width = 0,
                });
                return {};
            },
        });

    NodeRegistry::register_series(
        "partitioned_bars",
        [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
            std::vector<std::pair<InputId, Node::Input>> res;

            res.emplace_back("num_partitions", Node::Input{.id = "num_partitions", .display_name = "Number of Partitions", .valid_data_types = {DataType::INTEGER}});

            res.emplace_back("width", Node::Input{.id = "width", .display_name = "Width", .valid_data_types = {DataType::NUMBER}});

            res.emplace_back("x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::NUMBER_COLUMN}});

            int64_t num_series = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "num_partitions").value_or(0), int64_t{0}, int64_t{255});
            for (int64_t i = 0; i < num_series; i++) {
                std::string name = "partition_" + std::to_string(i);
                std::string name_color = "partition_color_" + std::to_string(i);
                res.emplace_back(name, Node::Input{.id = name, .display_name = "Partition " + std::to_string(i), .valid_data_types = {DataType::NUMBER_COLUMN}});
                res.emplace_back(name_color,
                                 Node::Input{.id = name_color, .display_name = "Partition " + std::to_string(i) + " Color", .valid_data_types = {DataType::COLOR}, .default_value = Color{1, 0, 0, 1}});
            }

            return res;
        },
        {
            .display_name = "Partitioned Bars",
            .get_limits = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s) -> ErrorOr<Limits> {
                auto x_col = TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "x", "Missing"))));
                auto x = Utils::find_limits(x_col);

                int64_t num_series = std::clamp(eng->try_data_type_conversion<int64_t>(TRY(Utils::try_find(s.data, "num_partitions", "Missing"))).value_or(0), int64_t{0}, int64_t{255});

                double width = TRY(eng->try_data_type_conversion<double>(TRY(Utils::try_find(s.data, "width", "Missing"))));

                double low = std::numeric_limits<double>::infinity();
                double high = -std::numeric_limits<double>::infinity();

                std::vector<double> sum_col;
                sum_col.resize(x_col.size());

                for (int64_t i = 0; i < num_series; i++) {
                    std::string name = "partition_" + std::to_string(i);
                    auto col = TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, name, "Missing Lim"))));

                    if (col.size() != x_col.size())
                        return ERR("Columns are not of the same size");

                    for (size_t i = 0; i < x_col.size(); i++)
                        sum_col[i] += col[i];
                }

                auto y = Utils::find_limits(sum_col);

                return Limits{
                    .x_low = x.first - width / 2,
                    .x_high = x.second + width / 2,
                    .y_low = 0,
                    .y_high = y.second,
                };
            },
            .evalulate = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s, Figure& fig, const FigureBounds& bounds) -> ErrorOr<void> {
                auto x_col = TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, "x", "Missing"))));

                std::vector<std::vector<double>> cols;
                std::vector<Color> colors;

                int64_t num_series = std::clamp(eng->try_data_type_conversion<int64_t>(TRY(Utils::try_find(s.data, "num_partitions", "Missing"))).value_or(0), int64_t{0}, int64_t{255});

                double width = TRY(eng->try_data_type_conversion<double>(TRY(Utils::try_find(s.data, "width", "Missing"))));

                for (int64_t i = 0; i < num_series; i++) {
                    std::string name = "partition_" + std::to_string(i);
                    std::string name_color = "partition_color_" + std::to_string(i);
                    cols.push_back(TRY(eng->try_data_type_conversion<std::vector<double>>(TRY(Utils::try_find(s.data, name, "Missing")))));
                    colors.push_back(TRY(eng->try_data_type_conversion<Color>(TRY(Utils::try_find(s.data, name_color, "Missing Color")))));
                }

                for (auto& c : cols) {
                    if (c.size() != x_col.size())
                        return ERR("Columns are not of the same size");
                }

                for (auto& v : x_col)

                    for (size_t i = 0; i < x_col.size(); i++) {
                        auto x = x_col[i];

                        double cur_sum = 0;

                        for (size_t j = 0; j < cols.size(); j++) {
                            auto v = cols[j][i];

                            auto [x1, y1] = bounds.normalize({(float)(x - width / 2), (float)cur_sum});
                            auto [x2, y2] = bounds.normalize({(float)(x + width / 2), (float)(cur_sum + v)});

                            cur_sum += v;

                            fig.commands.push_back(DrawCommands::Rect{
                                .a = Pos{x1, y1},
                                .b = Pos{x2, y2},
                                .color = colors[j],
                            });
                        }
                    }
                return {};
            },
        });
}

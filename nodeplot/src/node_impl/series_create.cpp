#include "error.h"
#include "node_registry.h"
#include "nodeplot.h"
#include "types.h"
#include "utils.h"
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

                for (size_t i = 0; i < x_col.size(); i++) {
                    auto [x, y] = bounds.normalize({(float)x_col[i], (float)y_col[i]});
                    fig.commands.push_back(DrawCommands::Circle{
                        .pos = Pos{(float)x, (float)y},
                        .r = std::visit(Utils::overloaded{
                                            [&](std::vector<double> vec) { return vec[i]; },
                                            [](double c) { return c; },
                                        },
                                        point_size),
                        .color = std::visit(Utils::overloaded{
                                                [&](std::vector<Color> vec) { return vec[i]; },
                                                [](Color c) { return c; },
                                            },
                                            color),
                    });
                }
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
}

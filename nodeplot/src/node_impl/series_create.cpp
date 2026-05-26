#include "error.h"
#include "node_registry.h"
#include "nodeplot.h"
#include "types.h"
#include "utils.h"

using namespace NodePlot;

/*

  std::visit(Utils::overloaded{
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
                  series)
 *
 */

void register_series_create() {

    NodeRegistry::register_series("scatter",
                                  [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                      return {
                                          {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::COLUMN}}},
                                          {"y", Node::Input{.id = "y", .display_name = "Y", .valid_data_types = {DataType::COLUMN}}},
                                          {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR}, .default_value = Color{.r = 1, .g = 0, .b = 0, .a = 1}}},
                                          {"point_size", Node::Input{.id = "point_size", .display_name = "Point Size", .valid_data_types = {DataType::NUMBER}, .default_value = 3.0}},
                                      };
                                  },
                                  {
                                      .display_name = "Scatter",
                                      .get_limits = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s) -> ErrorOr<Limits> {
                                          auto x = Utils::find_limits(TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "x", "Missing")))).as_numeric_column()).values);
                                          auto y = Utils::find_limits(TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "y", "Missing")))).as_numeric_column()).values);
                                          return Limits{
                                              .x_low = x.first,
                                              .x_high = x.second,
                                              .y_low = y.first,
                                              .y_high = y.second,
                                          };
                                      },
                                      .evalulate = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s, Figure& fig, const FigureBounds& bounds) -> ErrorOr<void> {
                                          auto x_col = TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "x", "Missing")))).as_numeric_column());
                                          auto y_col = TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "y", "Missing")))).as_numeric_column());
                                          auto color = TRY(eng->try_data_type_conversion<Color>(TRY(Utils::try_find(s.data, "color", "Missing"))));
                                          auto point_size = TRY(eng->try_data_type_conversion<double>(TRY(Utils::try_find(s.data, "point_size", "Missing"))));

                                          if (x_col.values.size() != y_col.values.size())
                                              return ERR("Columns are not of the same size");

                                          for (size_t i = 0; i < x_col.values.size(); i++) {
                                              auto [x, y] = bounds.normalize({(float)x_col.values[i], (float)y_col.values[i]});
                                              fig.commands.push_back(DrawCommands::Circle{
                                                  .pos = Pos{(float)x, (float)y},
                                                  .r = point_size,
                                                  .color = color,
                                              });
                                          }
                                          return {};
                                      },
                                  });

    NodeRegistry::register_series("line",
                                  [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                      return {
                                          {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::COLUMN}}},
                                          {"y", Node::Input{.id = "y", .display_name = "Y", .valid_data_types = {DataType::COLUMN}}},
                                          {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR}, .default_value = Color{.r = 1, .g = 0, .b = 0, .a = 1}}},
                                          {"stroke_width", Node::Input{.id = "stroke_width", .display_name = "Stroke Width", .valid_data_types = {DataType::NUMBER}, .default_value = 3.0}},
                                      };
                                  },
                                  {
                                      .display_name = "Line",
                                      .get_limits = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s) -> ErrorOr<Limits> {
                                          auto x = Utils::find_limits(TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "x", "Missing")))).as_numeric_column()).values);
                                          auto y = Utils::find_limits(TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "y", "Missing")))).as_numeric_column()).values);
                                          return Limits{
                                              .x_low = x.first,
                                              .x_high = x.second,
                                              .y_low = y.first,
                                              .y_high = y.second,
                                          };
                                      },
                                      .evalulate = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s, Figure& fig, const FigureBounds& bounds) -> ErrorOr<void> {
                                          auto x_col = TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "x", "Missing")))).as_numeric_column());
                                          auto y_col = TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "y", "Missing")))).as_numeric_column());
                                          auto color = TRY(eng->try_data_type_conversion<Color>(TRY(Utils::try_find(s.data, "color", "Missing"))));
                                          auto stroke_width = TRY(eng->try_data_type_conversion<double>(TRY(Utils::try_find(s.data, "stroke_width", "Missing"))));

                                          if (x_col.values.size() != y_col.values.size())
                                              return ERR("Columns are not of the same size");

                                          for (size_t i = 1; i < x_col.values.size(); i++) {
                                              auto [x1, y1] = bounds.normalize({(float)x_col.values[i - 1], (float)y_col.values[i - 1]});
                                              auto [x2, y2] = bounds.normalize({(float)x_col.values[i], (float)y_col.values[i]});
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
                {"x", Node::Input{.id = "x", .display_name = "X", .valid_data_types = {DataType::COLUMN}}},
                {"y_min", Node::Input{.id = "y_min", .display_name = "Y Min", .valid_data_types = {DataType::COLUMN}}},
                {"y_max", Node::Input{.id = "y_max", .display_name = "Y Max", .valid_data_types = {DataType::COLUMN}}},
                {"color", Node::Input{.id = "color", .display_name = "Color", .valid_data_types = {DataType::COLOR}, .default_value = Color{.r = 1, .g = 0, .b = 0, .a = 0.2}}},
            };
        },
        {
            .display_name = "Ribbon",
            .get_limits = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s) -> ErrorOr<Limits> {
                auto x = Utils::find_limits(TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "x", "Missing")))).as_numeric_column()).values);
                auto y_min = Utils::find_limits(TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "y_min", "Missing")))).as_numeric_column()).values);
                auto y_max = Utils::find_limits(TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "y_max", "Missing")))).as_numeric_column()).values);
                return Limits{
                    .x_low = x.first,
                    .x_high = x.second,
                    .y_low = std::min(y_min.first, y_max.first),
                    .y_high = std::max(y_min.second, y_max.second),
                };
            },
            .evalulate = [](NodePlotFile*, EvaluatedNodeGraph* eng, const GenericSeries& s, Figure& fig, const FigureBounds& bounds) -> ErrorOr<void> {
                auto x_col = TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "x", "Missing")))).as_numeric_column());
                auto y_min_col = TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "y_min", "Missing")))).as_numeric_column());
                auto y_max_col = TRY(TRY(eng->try_data_type_conversion<Column>(TRY(Utils::try_find(s.data, "y_max", "Missing")))).as_numeric_column());
                auto color = TRY(eng->try_data_type_conversion<Color>(TRY(Utils::try_find(s.data, "color", "Missing"))));

                if (x_col.values.size() != y_min_col.values.size() && x_col.values.size() != y_max_col.values.size())
                    return ERR("Columns are not of the same size");

                std::vector<Pos> points;
                for (size_t i = 0; i < x_col.values.size(); i++) {
                    auto [x, y] = bounds.normalize({(float)x_col.values[i], (float)y_min_col.values[i]});
                    points.push_back({(float)x, (float)y});
                }
                for (size_t i = x_col.values.size(); i > 0; i--) {
                    auto [x, y] = bounds.normalize({(float)x_col.values[i - 1], (float)y_max_col.values[i - 1]});
                    points.push_back({(float)x, (float)y});
                }
                if (x_col.values.size() > 0) {
                    auto [x, y] = bounds.normalize({(float)x_col.values[0], (float)y_min_col.values[0]});
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

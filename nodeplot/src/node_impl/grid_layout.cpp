#include "error.h"
#include "nodeplot.h"
#include "types.h"
#include "utils.h"
#include <vector>

using namespace NodePlot;

void register_grid_layout() {
    NodeRegistry::register_node("grid_layout",
                                Node{
                                    .type_id = "grid_layout",
                                    .display_name = "Grid Layout",
                                    .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                                        std::vector<std::pair<InputId, Node::Input>> res;

                                        res.emplace_back("cols", Node::Input{.id = "cols", .display_name = "Columns", .valid_data_types = {DataType::INTEGER}, .default_value = 1});
                                        res.emplace_back("rows", Node::Input{.id = "rows", .display_name = "Rows", .valid_data_types = {DataType::INTEGER}, .default_value = 1});

                                        int64_t cols = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "cols").value_or(0), int64_t{0}, int64_t{255});
                                        int64_t rows = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "rows").value_or(0), int64_t{0}, int64_t{255});

                                        for (int64_t j = 0; j < rows; j++) {
                                            for (int64_t i = 0; i < cols; i++) {
                                                std::string name = "figure_" + std::to_string(i) + "_" + std::to_string(j);
                                                res.emplace_back(
                                                    name, Node::Input{.id = name, .display_name = "Figure " + std::to_string(i) + " " + std::to_string(j), .valid_data_types = {DataType::FIGURE}});
                                            }
                                        }

                                        return res;
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"figure", Node::Output{.id = "figure", .display_name = "Figure", .valid_data_types = {DataType::FIGURE}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        int64_t cols = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "cols").value_or(0), int64_t{0}, int64_t{255});
                                        int64_t rows = std::clamp(eng->get_input_value<int64_t>(npf, node_id, "rows").value_or(0), int64_t{0}, int64_t{255});

                                        float width_per_col = 1.0f / cols;
                                        float height_per_row = 1.0f / rows;

                                        Figure res;

                                        for (size_t r = 0; r < rows; r++) {
                                            for (size_t c = 0; c < cols; c++) {

                                                auto transform_pos = [&](Pos p) {
                                                    return Pos{
                                                        p.x * width_per_col + c * width_per_col,
                                                        p.y * height_per_row + r * height_per_row,
                                                    };
                                                };

                                                auto fig = TRY(eng->get_input_value<Figure>(npf, node_id, "figure_" + std::to_string(c) + "_" + std::to_string(r)));
                                                for (auto& cmd : fig.commands) {
                                                    std::visit(NodePlot::Utils::overloaded{
                                                                   [&](DrawCommands::Line& l) {
                                                                       res.commands.push_back(DrawCommands::Line{
                                                                           .start = transform_pos(l.start),
                                                                           .end = transform_pos(l.end),
                                                                           .color = l.color,
                                                                           .stroke_width = l.stroke_width,
                                                                       });
                                                                   },
                                                                   [&](DrawCommands::Circle& c) {
                                                                       res.commands.push_back(DrawCommands::Circle{
                                                                           .pos = transform_pos(c.pos),
                                                                           .r = c.r,
                                                                           .color = c.color,
                                                                       });
                                                                   },
                                                                   [&](DrawCommands::Rect& r) {
                                                                       res.commands.push_back(DrawCommands::Rect{
                                                                           .a = transform_pos(r.a),
                                                                           .b = transform_pos(r.b),
                                                                           .color = r.color,
                                                                           .stroke_color = r.stroke_color,
                                                                           .stroke_width = r.stroke_width,
                                                                       });
                                                                   },
                                                                   [&](DrawCommands::Polygon& p) {
                                                                       std::vector<Pos> new_pos;
                                                                       new_pos.reserve(p.points.size());

                                                                       for (auto& point : p.points) {
                                                                           new_pos.push_back(transform_pos(point));
                                                                       }

                                                                       res.commands.push_back(DrawCommands::Polygon{
                                                                           .points = new_pos,
                                                                           .stroke_color = p.stroke_color,
                                                                           .fill_color = p.fill_color,
                                                                           .stroke_width = p.stroke_width,
                                                                       });
                                                                   },
                                                                   [&](DrawCommands::Text& t) {
                                                                       res.commands.push_back(DrawCommands::Text{
                                                                           .pos = transform_pos(t.pos),
                                                                           .text = t.text,
                                                                           .anchor = t.anchor,
                                                                           .font_size = t.font_size,
                                                                           .rotate = t.rotate,
                                                                       });
                                                                   },
                                                               },
                                                               cmd);
                                                }
                                            }
                                        }

                                        cache.computed_outputs["figure"] = res;

                                        return {};
                                    },
                                });
}

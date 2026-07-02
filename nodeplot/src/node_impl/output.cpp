#include "error.h"
#include "nodeplot.h"
#include "types.h"
#include "utils.h"

#include <sstream>

using namespace NodePlot;

void register_output() {
    NodeRegistry::register_node("output",
                                Node{
                                    .type_id = "output",
                                    .display_name = "Output",
                                    .inputs = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id) -> std::vector<std::pair<InputId, Node::Input>> {
                                        std::vector<std::pair<InputId, Node::Input>> res;

                                        res.emplace_back("filename", Node::Input{.id = "filename", .display_name = "Filename", .valid_data_types = {DataType::STRING}});

                                        res.emplace_back("width", Node::Input{.id = "width", .display_name = "Width", .valid_data_types = {DataType::NUMBER}, .default_value = 300.0});
                                        res.emplace_back("height", Node::Input{.id = "height", .display_name = "Height", .valid_data_types = {DataType::NUMBER}, .default_value = 300.0});

                                        res.emplace_back("figure", Node::Input{.id = "figure", .display_name = "Figure", .valid_data_types = {DataType::FIGURE}});

                                        return res;
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"svg", Node::Output{.id = "svg", .display_name = "SVG", .valid_data_types = {DataType::STRING}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        double width = TRY(eng->get_input_value<double>(npf, node_id, "width"));
                                        double height = TRY(eng->get_input_value<double>(npf, node_id, "height"));

                                        std::stringstream ss;
                                        ss << "<svg width=\"" << width << "\" height=\"" << height << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

                                        auto color_to_svg = [](Color c) -> std::string {
                                            return "rgb(" + std::to_string(((int)(c.r * 255.0))) + ", " + std::to_string(((int)(c.g * 255.0))) + ", " + std::to_string(((int)(c.b * 255.0))) + ")";
                                        };

                                        auto figure = TRY(eng->get_input_value<Figure>(npf, node_id, "figure"));
                                        for (auto& cmd : figure.commands) {
                                            std::visit(Utils::overloaded{
                                                           [&](DrawCommands::Line& l) {
                                                               ss << "<line x1=\"" << (l.start.x * width) << "\" y1=\"" << (l.start.y * height) << "\" x2=\"" << (l.end.x * width) << "\" y2=\""
                                                                  << (l.end.y * height) << "\" stroke=\"" << color_to_svg(l.color)
                                                                  << "\" stroke-opacity=\"" + std::to_string(l.color.a) + "\" stroke-width=\"" << l.stroke_width << "\" stroke-linecap=\"round\" />\n";
                                                           },
                                                           [&](DrawCommands::Circle& c) {
                                                               ss << "<circle cx=\"" << (c.pos.x * width) << "\" cy=\"" << (c.pos.y * height) << "\" r=\"" << c.r << "\" fill=\""
                                                                  << color_to_svg(c.color) << "\" fill-opacity=\"" << c.color.a << "\" />\n";
                                                           },
                                                           [&](DrawCommands::Rect& r) {
                                                               float x = std::min(r.a.x, r.b.x);
                                                               float y = std::min(r.a.y, r.b.y);
                                                               float w = std::max(r.a.x, r.b.x) - x;
                                                               float h = std::max(r.a.y, r.b.y) - y;
                                                               ss << "<rect x=\"" << (x * width) << "\" y=\"" << (y * height) << "\" width=\"" << (w * width) << "\" height=\"" << (h * height)
                                                                  << "\" fill=\"" << color_to_svg(r.color) << "\" fill-opacity=\"" << r.color.a << "\" stroke=\"" << color_to_svg(r.stroke_color)
                                                                  << "\" stroke-opacity=\"" + std::to_string(r.stroke_color.a) + "\" stroke-width=\"" << r.stroke_width << "\" />\n";
                                                           },
                                                           [&](DrawCommands::Polygon& p) {
                                                               ss << "<polygon points=\"";

                                                               bool first = true;
                                                               for (auto& point : p.points) {
                                                                   if (first) {
                                                                       first = false;
                                                                   } else {
                                                                       ss << ",";
                                                                   }
                                                                   ss << (point.x * width) << "," << (point.y * height);
                                                               }

                                                               ss << "\" fill-opacity=\"" << (p.fill_color.a * 100) << "%\" fill=\"" + color_to_svg(p.fill_color) + "\" stroke-opacity=\""
                                                                  << (p.stroke_color.a * 100) << "%\" stroke=\"" + color_to_svg(p.stroke_color) + "\"  />\n";
                                                           },
                                                           [&](DrawCommands::Text& t) {
                                                               ss << "<g transform=\"translate(" << (t.pos.x * width) << ", " << (t.pos.y * height) << ")\">\n";
                                                               ss << "<text font-family=\"sans-serif\" text-anchor=\"" + [&]() -> std::string {
                                                                   switch (t.anchor) {
                                                                   case NodePlot::DrawCommands::Text::LEFT:
                                                                       return "start";
                                                                   case NodePlot::DrawCommands::Text::MIDDLE:
                                                                       return "middle";
                                                                   case NodePlot::DrawCommands::Text::RIGHT:
                                                                       return "end";
                                                                   }
                                                               }() + "\" font-size=\"" << t.font_size
                                                                                       << "\" transform=\"rotate(" + std::to_string(t.rotate) + ")\" >";
                                                               ss << t.text << "";
                                                               ss << "</text> </g>\n";
                                                           },
                                                       },
                                                       cmd);
                                        }

                                        ss << "</svg>\n";

                                        // std::println("{}", ss.str());

                                        cache.computed_outputs["svg"] = ss.str();

                                        return {};
                                    },
                                });
}

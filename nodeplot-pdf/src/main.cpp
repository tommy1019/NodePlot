#include <fstream>
#include <numbers>
#include <ranges>
#include <string>
#include <variant>

#include <nodeplot/error.h>
#include <nodeplot/nodeplot.h>

#include <hpdf.h>

int main(int argc, char** argv) {

    if (argc != 2)
        REQUIRE_NOT_REACHED("Error: Incorrect number of arguments. Exactly one required");

    std::ifstream ifs(argv[1]);
    if (!ifs)
        REQUIRE_NOT_REACHED("Could not open input file");

    NodePlot::NodeRegistry::init();

    NodePlot::NodePlotFile npf = MUST(NodePlot::NodePlotFile::from_json(nlohmann::json::parse(ifs), argv[1]));
    auto main_graph = npf.graphs.find("main");
    if (main_graph == npf.graphs.end())
        REQUIRE_NOT_REACHED("Input file missing main graph");

    NodePlot::EvaluatedNodeGraph eng{.graph_id = "main"};

    for (auto& n : main_graph->second.nodes) {
        if (n.second.type_id == "output") {
            std::string filename = MUST(eng.get_input_value<std::string>(&npf, n.first, "filename")) + ".pdf";

            auto width = MUST(eng.get_input_value<double>(&npf, n.first, "width"));
            auto height = MUST(eng.get_input_value<double>(&npf, n.first, "height"));

            auto figure = MUST(eng.get_input_value<NodePlot::Figure>(&npf, n.first, "figure"));

            auto pdf = HPDF_New(nullptr, nullptr);
            REQUIRE(pdf, "Could not create pdf");

            auto page = HPDF_AddPage(pdf);
            HPDF_Page_SetWidth(page, width);
            HPDF_Page_SetHeight(page, height);

            for (auto& cmd : figure.commands) {
                std::visit(NodePlot::Utils::overloaded{
                               [&](NodePlot::DrawCommands::Line& cmd) {
                                   HPDF_Page_SetRGBStroke(page, cmd.color.r, cmd.color.g, cmd.color.b);
                                   HPDF_Page_SetLineCap(page, HPDF_ROUND_END);
                                   HPDF_Page_SetLineWidth(page, cmd.stroke_width);

                                   HPDF_ExtGState gstate = HPDF_CreateExtGState(pdf);
                                   HPDF_ExtGState_SetAlphaStroke(gstate, cmd.color.a);
                                   HPDF_Page_SetExtGState(page, gstate);

                                   HPDF_Page_MoveTo(page, cmd.start.x * width, (1.0 - cmd.start.y) * height);
                                   HPDF_Page_LineTo(page, cmd.end.x * width, (1.0 - cmd.end.y) * height);
                                   HPDF_Page_Stroke(page);
                               },
                               [&](NodePlot::DrawCommands::Circle& cmd) {
                                   HPDF_Page_SetRGBFill(page, cmd.color.r, cmd.color.g, cmd.color.b);

                                   HPDF_ExtGState gstate = HPDF_CreateExtGState(pdf);
                                   HPDF_ExtGState_SetAlphaFill(gstate, cmd.color.a);
                                   HPDF_Page_SetExtGState(page, gstate);

                                   HPDF_Page_Circle(page, cmd.pos.x * width, (1.0 - cmd.pos.y) * height, cmd.r);
                                   HPDF_Page_Fill(page);
                               },
                               [&](NodePlot::DrawCommands::Rect& cmd) {
                                   HPDF_Page_SetRGBFill(page, cmd.color.r, cmd.color.g, cmd.color.b);
                                   HPDF_Page_SetRGBStroke(page, cmd.stroke_color.r, cmd.stroke_color.g, cmd.stroke_color.b);
                                   HPDF_Page_SetLineCap(page, HPDF_ROUND_END);
                                   HPDF_Page_SetLineWidth(page, cmd.stroke_width);

                                   HPDF_ExtGState gstate = HPDF_CreateExtGState(pdf);
                                   HPDF_ExtGState_SetAlphaFill(gstate, cmd.color.a);
                                   HPDF_ExtGState_SetAlphaStroke(gstate, cmd.stroke_color.a);
                                   HPDF_Page_SetExtGState(page, gstate);

                                   double sx = std::min(cmd.a.x, cmd.b.x);
                                   double sy = std::max(cmd.a.y, cmd.b.y);
                                   double w = std::abs(cmd.a.x - cmd.b.x);
                                   double h = std::abs(cmd.a.y - cmd.b.y);

                                   HPDF_Page_Rectangle(page, sx * width, (1.0 - sy) * height, w * width, h * height);
                                   HPDF_Page_FillStroke(page);
                               },
                               [&](NodePlot::DrawCommands::Polygon& cmd) {
                                   if (cmd.points.empty())
                                       return;

                                   HPDF_Page_SetRGBFill(page, cmd.fill_color.r, cmd.fill_color.g, cmd.fill_color.b);
                                   HPDF_Page_SetRGBStroke(page, cmd.stroke_color.r, cmd.stroke_color.g, cmd.stroke_color.b);
                                   HPDF_Page_SetLineCap(page, HPDF_ROUND_END);
                                   HPDF_Page_SetLineWidth(page, cmd.stroke_width);

                                   HPDF_ExtGState gstate = HPDF_CreateExtGState(pdf);
                                   HPDF_ExtGState_SetAlphaFill(gstate, cmd.fill_color.a);
                                   HPDF_ExtGState_SetAlphaStroke(gstate, cmd.stroke_color.a);
                                   HPDF_Page_SetExtGState(page, gstate);

                                   HPDF_Page_MoveTo(page, cmd.points.front().x * width, (1.0 - cmd.points.front().y) * height);

                                   for (auto& p : cmd.points | std::ranges::views::drop(1)) {
                                       HPDF_Page_LineTo(page, p.x * width, (1.0 - p.y) * height);
                                   }

                                   HPDF_Page_ClosePath(page);

                                   HPDF_Page_FillStroke(page);
                               },
                               [&](NodePlot::DrawCommands::Text& cmd) {
                                   HPDF_Page_GSave(page);

                                   double rot = -cmd.rotate * std::numbers::pi / 180.0;
                                   HPDF_Page_Concat(page, std::cos(rot), std::sin(rot), -std::sin(rot), std::cos(rot), cmd.pos.x * width, (1.0 - cmd.pos.y) * height);

                                   HPDF_Page_SetRGBFill(page, 0.0f, 0.0f, 0.0f);
                                   HPDF_Page_SetRGBStroke(page, 0.0f, 0.0f, 0.0f);

                                   HPDF_ExtGState gstate = HPDF_CreateExtGState(pdf);
                                   HPDF_ExtGState_SetAlphaFill(gstate, 1.0f);
                                   HPDF_ExtGState_SetAlphaStroke(gstate, 1.0f);
                                   HPDF_Page_SetExtGState(page, gstate);

                                   HPDF_Font font;

                                   if (!cmd.bold)
                                       font = HPDF_GetFont(pdf, "Helvetica", NULL);
                                   else
                                       font = HPDF_GetFont(pdf, "Helvetica-Bold", NULL);

                                   HPDF_Page_SetFontAndSize(page, font, cmd.font_size);

                                   HPDF_Page_BeginText(page);

                                   switch (cmd.anchor) {
                                   case NodePlot::DrawCommands::Text::LEFT:
                                       HPDF_Page_TextRect(page, 0, cmd.font_size, width, 0, cmd.text.c_str(), HPDF_TALIGN_LEFT, nullptr);
                                       break;
                                   case NodePlot::DrawCommands::Text::MIDDLE:
                                       HPDF_Page_TextRect(page, -width, cmd.font_size, width, 0, cmd.text.c_str(), HPDF_TALIGN_CENTER, nullptr);
                                       break;
                                   case NodePlot::DrawCommands::Text::RIGHT:
                                       HPDF_Page_TextRect(page, -width, cmd.font_size, 0, 0, cmd.text.c_str(), HPDF_TALIGN_RIGHT, nullptr);
                                       break;
                                   }

                                   HPDF_Page_EndText(page);

                                   HPDF_Page_GRestore(page);
                               },
                           },
                           cmd);
            }

            printf("Saving to file '%s'\n", filename.c_str());
            REQUIRE(HPDF_SaveToFile(pdf, filename.c_str()) == 0, "Failed to write pdf to file");
            HPDF_Free(pdf);
        }
    }
}
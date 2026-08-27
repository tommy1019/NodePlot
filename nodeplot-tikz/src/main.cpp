#include <fstream>
#include <string>
#include <variant>

#include <nodeplot/error.h>
#include <nodeplot/nodeplot.h>

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
            std::string filename = MUST(eng.get_input_value<std::string>(&npf, n.first, "filename")) + ".tex";

            auto width = MUST(eng.get_input_value<double>(&npf, n.first, "width"));
            auto height = MUST(eng.get_input_value<double>(&npf, n.first, "height"));

            auto figure = MUST(eng.get_input_value<NodePlot::Figure>(&npf, n.first, "figure"));

            FILE* file = fopen(filename.c_str(), "w");

            fprintf(file, "\\begin{tikzpicture}\n");

            for (auto& cmd : figure.commands) {
                std::visit(NodePlot::Utils::overloaded{
                               [&](NodePlot::DrawCommands::Line& cmd) {
                                   fprintf(file,
                                           "    \\draw[color={rgb,255:red,%d; green,%d; blue,%d}, opacity=%f, line width=%fmm] (%fmm,%fmm) -- (%fmm,%fmm);\n",
                                           (int)std::floor(cmd.color.r * 255),
                                           (int)std::floor(cmd.color.g * 255),
                                           (int)std::floor(cmd.color.b * 255),
                                           cmd.color.a,
                                           cmd.stroke_width,
                                           cmd.start.x * width,
                                           (1.0 - cmd.start.y) * height,
                                           cmd.end.x * width,
                                           (1.0 - cmd.end.y) * height);
                               },
                               [&](NodePlot::DrawCommands::Circle& cmd) {
                                   fprintf(file,
                                           "    \\fill[color={rgb,255:red,%d; green,%d; blue,%d}, opacity=%f, fill opacity=0.5] (%fmm,%fmm) circle (%fmm);\n",
                                           (int)std::floor(cmd.color.r * 255),
                                           (int)std::floor(cmd.color.g * 255),
                                           (int)std::floor(cmd.color.b * 255),
                                           cmd.color.a,
                                           cmd.pos.x * width,
                                           (1.0 - cmd.pos.y) * height,
                                           cmd.r);
                               },
                               [&](NodePlot::DrawCommands::Rect& cmd) {
                                   fprintf(file,
                                           "    \\draw[draw={rgb,255:red,%d; green,%d; blue,%d}, draw opacity=%f, line width=%fmm, fill={rgb,255:red,%d; green,%d; blue,%d}, fill opacity=%f] "
                                           "(%fmm,%fmm) rectangle (%fmm,%fmm);\n",
                                           (int)std::floor(cmd.stroke_color.r * 255),
                                           (int)std::floor(cmd.stroke_color.g * 255),
                                           (int)std::floor(cmd.stroke_color.b * 255),
                                           cmd.stroke_color.a,
                                           cmd.stroke_width,
                                           (int)std::floor(cmd.color.r * 255),
                                           (int)std::floor(cmd.color.g * 255),
                                           (int)std::floor(cmd.color.b * 255),
                                           cmd.color.a,
                                           cmd.a.x * width,
                                           (1.0 - cmd.a.y) * height,
                                           cmd.b.x * width,
                                           (1.0 - cmd.b.y) * height);
                               },
                               [&](NodePlot::DrawCommands::Polygon& cmd) {
                                   if (cmd.points.empty())
                                       return;

                                   fprintf(file,
                                           "    \\draw[draw={rgb,255:red,%d; green,%d; blue,%d}, draw opacity=%f, line width=%fmm, fill={rgb,255:red,%d; green,%d; blue,%d}, fill opacity=%f] ",
                                           (int)std::floor(cmd.stroke_color.r * 255),
                                           (int)std::floor(cmd.stroke_color.g * 255),
                                           (int)std::floor(cmd.stroke_color.b * 255),
                                           cmd.stroke_color.a,
                                           cmd.stroke_width,
                                           (int)std::floor(cmd.fill_color.r * 255),
                                           (int)std::floor(cmd.fill_color.g * 255),
                                           (int)std::floor(cmd.fill_color.b * 255),
                                           cmd.fill_color.a);

                                   fprintf(file, "(%fmm,%fmm)", cmd.points.front().x * width, (1.0 - cmd.points.front().y) * height);
                                   for (auto& p : cmd.points | std::ranges::views::drop(1)) {
                                       fprintf(file, " -- (%fmm,%fmm)", p.x * width, (1.0 - p.y) * height);
                                   }
                                   fprintf(file, " -- cycle;\n");
                               },
                               [&](NodePlot::DrawCommands::Text& cmd) {
                                   fprintf(file,
                                           "\\node[rotate=%f, anchor=%s, font={\\fontsize{%f}{%f}\\selectfont}] at (%fmm,%fmm) {%s%s%s};\n",
                                           -cmd.rotate,
                                           cmd.anchor == NodePlot::DrawCommands::Text::LEFT ? "base west" : (cmd.anchor == NodePlot::DrawCommands::Text::RIGHT ? "base east" : "base"),
                                           cmd.font_size * (1.0 / 0.35),
                                           cmd.font_size * (1.0 / 0.35),
                                           cmd.pos.x * width,
                                           (1.0 - cmd.pos.y) * height,
                                           cmd.bold ? "\\textbf{" : "",
                                           cmd.text.c_str(),
                                           cmd.bold ? "}" : "");

                                   //    HPDF_Page_GSave(page);

                                   //    double rot = -cmd.rotate * std::numbers::pi / 180.0;
                                   //    HPDF_Page_Concat(page, std::cos(rot), std::sin(rot), -std::sin(rot), std::cos(rot), cmd.pos.x * width, (1.0 - cmd.pos.y) * height);

                                   //    HPDF_Page_SetRGBFill(page, 0.0f, 0.0f, 0.0f);
                                   //    HPDF_Page_SetRGBStroke(page, 0.0f, 0.0f, 0.0f);

                                   //    HPDF_ExtGState gstate = HPDF_CreateExtGState(pdf);
                                   //    HPDF_ExtGState_SetAlphaFill(gstate, 1.0f);
                                   //    HPDF_ExtGState_SetAlphaStroke(gstate, 1.0f);
                                   //    HPDF_Page_SetExtGState(page, gstate);

                                   //    HPDF_Font font;

                                   //    if (!cmd.bold)
                                   //        font = HPDF_GetFont(pdf, "Helvetica", NULL);
                                   //    else
                                   //        font = HPDF_GetFont(pdf, "Helvetica-Bold", NULL);

                                   //    HPDF_Page_SetFontAndSize(page, font, cmd.font_size);

                                   //    HPDF_Page_BeginText(page);

                                   //    switch (cmd.anchor) {
                                   //    case NodePlot::DrawCommands::Text::LEFT:
                                   //        HPDF_Page_TextRect(page, 0, cmd.font_size, width, 0, cmd.text.c_str(), HPDF_TALIGN_LEFT, nullptr);
                                   //        break;
                                   //    case NodePlot::DrawCommands::Text::MIDDLE:
                                   //        HPDF_Page_TextRect(page, -width, cmd.font_size, width, 0, cmd.text.c_str(), HPDF_TALIGN_CENTER, nullptr);
                                   //        break;
                                   //    case NodePlot::DrawCommands::Text::RIGHT:
                                   //        HPDF_Page_TextRect(page, -width, cmd.font_size, 0, 0, cmd.text.c_str(), HPDF_TALIGN_RIGHT, nullptr);
                                   //        break;
                                   //    }

                                   //    HPDF_Page_EndText(page);

                                   //    HPDF_Page_GRestore(page);
                               },
                           },
                           cmd);
            }

            fprintf(file, "\\end{tikzpicture}\n");
        }
    }
}
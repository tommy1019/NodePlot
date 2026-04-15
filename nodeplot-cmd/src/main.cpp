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
            std::string filename = MUST(eng.get_input_value<std::string>(&npf, n.first, "filename"));
            auto svg = MUST(eng.get_output_data(&npf, n.first, "svg"));

            if (!std::holds_alternative<std::string>(svg))
                REQUIRE_NOT_REACHED("SVG is not a string type");

            std::ofstream out(filename);
            REQUIRE(out, "Failed to open output file: {}", filename);
            out << std::get<std::string>(svg);
            out.close();
        }
    }
}
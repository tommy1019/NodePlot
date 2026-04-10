#include <fstream>
#include <variant>

#include <nodeplot/error.h>
#include <nodeplot/nodeplot.h>

int main(int argc, char** argv) {

    if (argc != 2)
        REQUIRE_NOT_REACHED("Error: Incorrect number of arguments. Exactly two required");

    EvaluatedNodeGraph eng = {
        .node_file = MUST(NodePlotFile::read(argv[1])),
    };

    for (auto& n : eng.node_file.node_graph("main").nodes) {
        if (std::holds_alternative<OutputNode>(n.second)) {
            auto& node = std::get<OutputNode>(n.second);

            std::string filename = MUST(eng.get_input("main", node.i_output_filename));

            std::ofstream out(filename);
            REQUIRE(out, "Failed to open output file: {}", filename);
            out << MUST(node.get_svg(&eng));
            out.close();
        }
    }
}
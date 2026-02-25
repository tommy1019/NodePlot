#include "../render_node.h"
#include "nodeplot/nodeplot.h"

template <>
void node_render(RenderNodeGraph* rng, SampledPropertyExtractNode& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input, auto input_custom) {
            input("##i_x", "##ii_x", "X", node.i_x);
            input("##i_y", "##ii_y", "Y", node.i_y);
        },
        [&](auto output) {
            output("##o_x", 0, "X Values");
            output("##o_min", 1, "Minimum");
            output("##o_avg", 2, "Average");
            output("##o_stdev", 3, "Standard Deviation");
            output("##o_max", 4, "Maximum");
        });
}

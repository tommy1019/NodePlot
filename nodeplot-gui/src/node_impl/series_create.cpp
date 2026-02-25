#include "../render_node.h"
#include "nodeplot/nodeplot.h"

template <>
void node_render(RenderNodeGraph* rng, ScatterSeriesCreateNode& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            input("##i_x", "##ii_x", "X", node.i_x);
            input("##i_y", "##ii_y", "Y", node.i_y);
            input("##i_color", "##ii_color", "Color", node.i_color);
            input("##i_point_size", "##ii_point_size", "Point Size", node.i_point_size);
        },
        [&](auto output) { output("##o_series", 0, "Series"); });
}

template <>
void node_render(RenderNodeGraph* rng, LineSeriesCreateNode& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            input("##i_x", "##ii_x", "X", node.i_x);
            input("##i_y", "##ii_y", "Y", node.i_y);
            input("##i_color", "##ii_color", "Color", node.i_color);
            input("##i_stroke_width", "##ii_stroke_width", "Stroke Width", node.i_stroke_width);
        },
        [&](auto output) { output("##o_series", 0, "Series"); });
}

template <>
void node_render(RenderNodeGraph* rng, RibbonSeriesCreateNode& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            input("##i_x", "##ii_x", "X", node.i_x);
            input("##i_y_min", "##ii_y_min", "Y Min", node.i_y_min);
            input("##i_y_max", "##ii_y_max", "Y Max", node.i_y_max);
            input("##i_color", "##ii_color", "Color", node.i_color);
        },
        [&](auto output) { output("##o_series", 0, "Series"); });
}
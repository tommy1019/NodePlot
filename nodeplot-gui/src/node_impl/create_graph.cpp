#include "../render_node.h"
#include "nodeplot/nodeplot.h"

template <>
void node_render(RenderNodeGraph* rng, CreateGraphNode& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            input("##i_series", "##ii_series", "Series", node.i_series);
            input("##i_title", "##ii_title", "Title", node.i_title);
            input("##i_xlab", "##ii_xlab", "X Label", node.i_xlab);
            input("##i_ylab", "##ii_ylab", "Y Label", node.i_ylab);
            input("##i_x_axis_log_scale", "##ii_x_axis_log_scale", "X Log Scale", node.i_x_axis_log_scale);
            input("##i_y_axis_log_scale", "##ii_y_axis_log_scale", "Y Log Scale", node.i_y_axis_log_scale);
            input("##i_style", "##ii_style", "Style", node.i_style);
        },
        [&](auto output) { output("##o_graph", 0, "Graph"); });
}

template <>
void node_render(RenderNodeGraph* rng, CreateGraphStyleNode& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            input("##i_plot_margines", "##ii_plot_margines", "Plot Margines", node.i_plot_margines);
            input("##i_internal_plot_margines", "##ii_internal_plot_margines", "Internal Plot Margines", node.i_internal_plot_margines);
            input("##i_x_axis_tick_mark_font_size", "##ii_x_axis_tick_mark_font_size", "X Axis Tick Mark Font Size", node.i_x_axis_tick_mark_font_size);
            input("##i_x_axis_tick_mark_size", "##ii_x_axis_tick_mark_size", "X Axis Tick Mark Size", node.i_x_axis_tick_mark_size);
            input("##i_x_axis_label_font_size", "##ii_x_axis_label_font_size", "X Axis Label Font Size", node.i_x_axis_label_font_size);
            input("##i_y_axis_tick_mark_font_size", "##ii_y_axis_tick_mark_font_size", "Y Axis Tick Mark Font Size", node.i_y_axis_tick_mark_font_size);
            input("##i_y_axis_tick_mark_size", "##ii_y_axis_tick_mark_size", "Y Axis Tick Mark Size", node.i_y_axis_tick_mark_size);
            input("##i_y_axis_label_font_size", "##ii_y_axis_label_font_size", "Y Axis Label Font Size", node.i_y_axis_label_font_size);
            input("##i_title_font_size", "##ii_title_font_size", "Title Font Size", node.i_title_font_size);
        },
        [&](auto output) { output("##o_graph", 0, "Graph"); });
}

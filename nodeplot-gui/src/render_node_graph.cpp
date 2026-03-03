#include "render_node_graph.h"

#include <set>

void RenderNodeGraph::update_target(NodeId updated_node) {
    if (updated_node < 0) {
        eval_node_graph.loaded_nodes.clear();
        render_nodes.clear();
    } else {
        // TODO: Theres probabaly a faster way to do this
        std::set<NodeId> nodes_to_clear = {updated_node};

        bool added = false;
        do {
            added = false;
            for (auto& n : eval_node_graph.node_graph.nodes()) {
                std::visit(
                    [&](auto& n) {
                        auto dep_nodes = eval_node_graph.dependent_nodes(n.id);
                        if (!dep_nodes.has_value())
                            return;

                        for (auto other : dep_nodes.value()) {
                            if (nodes_to_clear.contains(n.id))
                                continue;

                            if (nodes_to_clear.contains(other)) {
                                nodes_to_clear.insert(n.id);
                                added = true;
                                break;
                            }
                        }
                    },
                    n.second);
            }
        } while (added);

        for (auto& n : nodes_to_clear) {
            eval_node_graph.loaded_nodes.erase(n);
            render_nodes.erase(n);
        }
    }

    auto set_err_svg = [&]() {
        svg_renderer->set_svg(R"(  
                        <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                        <text x="50" y="50" font-family="sans-serif" text-anchor="middle" font-size="12">
                            Error
                        </text>
                        </svg>
                    )");
    };

    if (auto viz_node = eval_node_graph.node_graph.nodes().find(eval_node_graph.node_graph.visualize_node()); viz_node != eval_node_graph.node_graph.nodes().end()) {
        if (std::holds_alternative<OutputNode>(viz_node->second)) {
            auto svg_or_error = std::get<OutputNode>(viz_node->second).get_svg(&eval_node_graph);
            if (svg_or_error.has_value()) {
                svg_renderer->set_svg(*svg_or_error);
                eval_node_graph.loaded_nodes[viz_node->first].error_message = {};
            } else {
                eval_node_graph.loaded_nodes[viz_node->first].error_message = svg_or_error.error();
                set_err_svg();
            }
        }
    } else {
        set_err_svg();
    }
}

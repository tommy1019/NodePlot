#include "render_node_graph.h"
#include "nodeplot/nodeplot.h"

#include <set>
#include <stack>

void RenderNodeGraph::update_target(GlobalNodeId updated_node) {

    bool updated_main_output = false;

    if (updated_node.id < 0) {
        eval_node_graph.loaded_nodes.clear();
        render_nodes.clear();
        updated_main_output = true;
    } else {
        // TODO: Theres probabaly a faster way to do this
        std::set<GlobalNodeId> nodes_to_clear = {updated_node};
        std::stack<GlobalNodeId> nodes_to_check;
        nodes_to_check.push(updated_node);

        while (!nodes_to_check.empty()) {
            auto cur_node_id = nodes_to_check.top();
            nodes_to_check.pop();

            for (auto& n : eval_node_graph.node_file.node_graph(cur_node_id.graph_name).nodes) {
                std::visit(
                    [&](auto& n) {
                        auto gid = GlobalNodeId{.id = n.id, .graph_name = cur_node_id.graph_name};
                        auto dep_nodes = eval_node_graph.dependent_nodes(gid);
                        if (!dep_nodes.has_value())
                            return;

                        for (auto other : dep_nodes.value()) {
                            if (other == cur_node_id) {
                                if (nodes_to_clear.insert(gid).second) {
                                    nodes_to_check.push(gid);
                                }
                            }
                        }
                    },
                    n.second);
            }
        }

        for (auto& n : nodes_to_clear) {
            eval_node_graph.loaded_nodes.erase(n);
            render_nodes.erase(n);
        }

        if (nodes_to_clear.contains(GLOBAL_NODE_ID_OUTPUT))
            updated_main_output = true;
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

    if (updated_main_output) {
        auto svg_or_error = std::get<OutputNode>(eval_node_graph.node_file.main_graph().nodes.at(NODE_ID_OUTPUT)).get_svg(&eval_node_graph);
        if (svg_or_error.has_value()) {
            svg_renderer->set_svg(*svg_or_error);
            eval_node_graph.loaded_nodes[GLOBAL_NODE_ID_OUTPUT].error_message = {};
        } else {
            eval_node_graph.loaded_nodes[GLOBAL_NODE_ID_OUTPUT].error_message = svg_or_error.error();
            set_err_svg();
        }
    } else {
        set_err_svg();
    }
}

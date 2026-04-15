#include "eval_node_graph.h"

#include "nodeplot.h"

namespace NodePlot {

ErrorOr<std::reference_wrapper<NodeGraph>> EvaluatedNodeGraph::node_graph(NodePlotFile* npf) {
    auto ng = npf->graphs.find(graph_id);
    if (ng == npf->graphs.end())
        return ERR("Invalid graph id");
    return ng->second;
}

ErrorOr<Data> EvaluatedNodeGraph::get_output_data(NodePlotFile* npf, NodeId node_id, OutputId output_id) {
    OutputCache& output_cache = cache[node_id];

    auto data = output_cache.computed_outputs.find(output_id);
    if (data != output_cache.computed_outputs.end()) {
        return data->second;
    }

    auto& ng = TRY(node_graph(npf)).get();
    NodeGraph::NodeStorage& node_input_storage = TRY(Utils::try_find(ng.nodes, node_id, "Invalid NodeID")).get();
    Node& node = TRY(Utils::try_find(NodeRegistry::node_map, node_input_storage.type_id, "Invalid NodeTypeId")).get();

    auto eval_err = node.evalulate(npf, this, node_id, output_cache);
    if (!eval_err.has_value()) {
        output_cache.error = eval_err.error();
    }

    return TRY(Utils::try_find(output_cache.computed_outputs, output_id, "OutputId not found")).get();
}

ErrorOr<Data> EvaluatedNodeGraph::get_input_data(NodePlotFile* npf, NodeId node_id, InputId input_id) {
    auto& ng = TRY(node_graph(npf)).get();
    NodeGraph::NodeStorage& node_input_storage = TRY(Utils::try_find(ng.nodes, node_id, "Invalid NodeID")).get();
    auto& input_storage = TRY(Utils::try_find(node_input_storage.input_storage, input_id, "Invalid InputID: " + input_id)).get();

    return std::visit(Utils::overloaded{
                          [&](Data data) -> ErrorOr<Data> { return data; },
                          [&](NodeGraph::InputPin input_pin) -> ErrorOr<Data> { return this->get_output_data(npf, input_pin.node_id, input_pin.output_id); },
                      },
                      input_storage);
}

} // namespace NodePlot
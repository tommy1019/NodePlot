#include "file.h"

namespace NodePlot {

ErrorOr<NodePlotFile> NodePlotFile::create(std::filesystem::path path) {
    NodePlotFile res;
    res.path = path;
    res.graphs = {{"main", NodeGraph{}}};
    return res;
}

ErrorOr<NodePlotFile> NodePlotFile::from_json(nlohmann::json json, std::filesystem::path path) {
    NodePlotFile res;
    res.path = path;

    auto graphs = json.find("graphs");
    if (graphs == json.end())
        return ERR("Missing graphs");

    for (auto it = graphs.value().begin(); it != graphs.value().end(); it++) {
        res.graphs[it.key()] = TRY(NodeGraph::from_json(it.value()));
    }

    return res;
}

ErrorOr<nlohmann::json> NodePlotFile::to_json() {
    nlohmann::json graphs;

    for (auto [graph_id, graph] : this->graphs) {
        graphs[graph_id] = TRY(graph.to_json());
    }

    nlohmann::json res;
    res["graphs"] = graphs;

    return res;
};

} // namespace NodePlot
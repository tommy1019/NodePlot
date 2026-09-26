#include "file.h"
#include "error.h"
#include "nlohmann/json_fwd.hpp"
#include "nodeplot.h"
#include "utils.h"
#include <functional>

namespace NodePlot {

ErrorOr<NodePlotFile> NodePlotFile::create(std::filesystem::path path) {
    NodePlotFile res;
    res.path = path;
    res.graphs = {{"main",
                   NodeGraph{
                       .nodes = {{0, NodePlot::NodeGraph::NodeStorage{.type_id = "output", .input_storage = {{"width", 300.0}, {"height", 300.0}, {"filename", "out"}}}}},
                       .next_free_node_id = 1,
                   }}};
    return res;
}

ErrorOr<NodePlotFile> NodePlotFile::from_json(nlohmann::json json, std::filesystem::path path) {

    {

        auto try_find = [](nlohmann::json& map, auto key, std::string err) -> ErrorOr<std::reference_wrapper<nlohmann::json>> {
            auto res = map.find(key);
            if (res == map.end())
                return ERR(err);
            return *res;
        };

        std::string version = "0.0.0";
        auto version_json = json.find("version");
        if (version_json != json.end() && version_json->is_string()) {
            version = version_json->get<std::string>();
        }

        if (version == "0.0.0") {
            if (auto err = [&]() -> ErrorOr<void> {
                    auto& graphs = TRY(try_find(json, "graphs", "")).get();

                    for (auto& graph : graphs) {
                        auto& nodes = TRY(try_find(graph, "nodes", "")).get();

                        for (auto& node : nodes) {
                            auto type_id = TRY(try_find(node, "type_id", "")).get();
                            auto& inputs = TRY(try_find(node, "inputs", "")).get();

                            if (type_id == "create_plot_style") {
                                nlohmann::json new_inputs;
                                for (auto& [key, input] : inputs.items()) {
                                    new_inputs["style_" + key] = input;
                                }
                                inputs = new_inputs;
                            }
                        }
                    }

                    return {};
                }();
                !err.has_value()) {
                return ERR("Could not upgrade from version 0.0.0: " + err.error());
            };
        }
    }

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
    res["version"] = NODEPLOT_VERSION;
    res["graphs"] = graphs;

    return res;
};

} // namespace NodePlot
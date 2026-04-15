#pragma once

#include <filesystem>
#include <map>

#include <nlohmann/json.hpp>

#include "node_graph.h"
#include "types.h"

namespace NodePlot {

struct NodePlotFile {
    std::filesystem::path path;
    std::map<GraphId, NodeGraph> graphs;

    static ErrorOr<NodePlotFile> create(std::filesystem::path path);

    static ErrorOr<NodePlotFile> from_json(nlohmann::json json, std::filesystem::path path);

    ErrorOr<nlohmann::json> to_json();
};

} // namespace NodePlot
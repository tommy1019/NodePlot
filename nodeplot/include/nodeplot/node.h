#pragma once

#include <functional>

#include "eval_node_graph.h"
#include "types.h"

namespace NodePlot {

namespace InputAttribute {

struct FilePath {};

struct AutoFillsFromTable {
    InputId id;
};

struct FunctionName {};

} // namespace InputAttribute

struct Node {
    using InputAttribute = std::variant<InputAttribute::FilePath, InputAttribute::AutoFillsFromTable, InputAttribute::FunctionName>;

    struct Input {
        InputId id;
        std::string display_name;
        std::vector<DataType> valid_data_types;
        std::optional<Data> default_value;
        std::vector<InputAttribute> attributes = {};
    };

    struct Output {
        OutputId id;
        std::string display_name;
        std::vector<DataType> valid_data_types;
    };

    NodeTypeId type_id;
    std::string display_name;

    std::function<ErrorOr<std::vector<std::pair<InputId, Input>>>(NodePlotFile*, EvaluatedNodeGraph*, NodeId)> inputs;
    std::function<ErrorOr<std::vector<std::pair<OutputId, Output>>>(NodePlotFile*, EvaluatedNodeGraph*, NodeId)> outputs;

    std::function<ErrorOr<void>(NodePlotFile*, EvaluatedNodeGraph*, NodeId, EvaluatedNodeGraph::OutputCache&)> evaluate;
};

} // namespace NodePlot
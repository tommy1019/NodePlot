#pragma once

#include <expected>
#include <functional>
#include <set>

#include "error.h"
#include "types.h"

#define EVAL_MUST(...)                                                                                                                                                                                 \
    ({                                                                                                                                                                                                 \
        auto RESULT = __VA_ARGS__;                                                                                                                                                                     \
        if (!RESULT.has_value()) {                                                                                                                                                                     \
            std::print("Failed to evaluate node value: {}\n", RESULT.error().description);                                                                                                             \
            for (auto input : RESULT.error().input_errors)                                                                                                                                             \
                std::print("    Error from: {}\n", input);                                                                                                                                             \
        }                                                                                                                                                                                              \
        *std::move(RESULT);                                                                                                                                                                            \
    })

namespace NodePlot {

struct EvaluatedNodeGraph;

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

    std::vector<std::string> toolbar_path;

    bool deletable = true;
    bool cloneable = true;

    using InputGenerator = std::function<ErrorOr<std::vector<std::pair<InputId, Input>>>(NodePlotFile*, EvaluatedNodeGraph*, NodeId)>;
    using OutputGenerator = std::function<ErrorOr<std::vector<std::pair<OutputId, Output>>>(NodePlotFile*, EvaluatedNodeGraph*, NodeId)>;

    InputGenerator inputs;
    OutputGenerator outputs;

    struct EvaluationError {
        std::string description;
        std::set<InputId> input_errors;
    };
    using EvaluateResult = std::expected<void, EvaluationError>;
    using EvaluateData = std::expected<Data, EvaluationError>;

    std::function<EvaluateResult(NodePlotFile*, EvaluatedNodeGraph*, NodeId, std::map<OutputId, Data>&)> evaluate;
};

} // namespace NodePlot

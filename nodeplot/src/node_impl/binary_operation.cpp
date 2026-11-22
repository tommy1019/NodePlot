#include "error.h"
#include "nodeplot.h"
#include "types.h"
#include <expected>
#include <vector>

using namespace NodePlot;

void register_binary_operation() {
    NodeRegistry::register_node(
        "binary_operation",
        Node{
            .type_id = "binary_operation",
            .display_name = "Binary Operation",
            .toolbar_path = {"Math", "Binary Operation"},
            .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                return {
                    {"operation", Node::Input{.id = "operation", .display_name = "Operation", .valid_data_types = {DataType::STRING}, .default_value = "+"}},
                    {"a", Node::Input{.id = "a", .display_name = "A Value", .valid_data_types = {DataType::NUMBER, DataType::STRING, DataType::NUMBER_COLUMN, DataType::STRING_COLUMN}}},
                    {"b", Node::Input{.id = "b", .display_name = "B Value", .valid_data_types = {DataType::NUMBER, DataType::STRING, DataType::NUMBER_COLUMN, DataType::STRING_COLUMN}}},
                };
            },
            .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                return {
                    {"result", Node::Output{.id = "result", .display_name = "Result", .valid_data_types = {DataType::NUMBER, DataType::STRING, DataType::NUMBER_COLUMN, DataType::STRING_COLUMN}}},
                };
            },
            .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, std::map<OutputId, Data>& output) -> Node::EvaluateResult {
                std::string operation = TRY(eng->get_input_value<std::string>(npf, node_id, "operation", true));

                auto a_val = TRY(eng->get_input_value_variant<double, std::string, std::vector<double>, std::vector<std::string>>(npf, node_id, "a", true));
                auto b_val = TRY(eng->get_input_value_variant<double, std::string, std::vector<double>, std::vector<std::string>>(npf, node_id, "b", true));

                auto op = [&operation](auto a, auto b) -> std::expected<decltype(a), Node::EvaluationError> {
                    if (operation == "+")
                        return a + b;
                    if (operation == "-")
                        return a - b;
                    if (operation == "*")
                        return a * b;
                    if (operation == "/")
                        return a / b;
                    return std::unexpected(Node::EvaluationError{
                        .description = "Unknown operation",
                        .input_errors = {"operation"},
                    });
                };

                output["result"] = TRY(std::visit(Utils::overloaded{
                                                      [&](std::vector<double> a, std::vector<double> b) -> Node::EvaluateData {
                                                          if (a.size() != b.size())
                                                              return std::unexpected(Node::EvaluationError{
                                                                  .description = "Columns 'A' and 'B' are of different size",
                                                                  .input_errors = {"a", "b"},
                                                              });

                                                          std::vector<double> res;
                                                          res.reserve(a.size());

                                                          for (size_t i = 0; i < a.size(); i++)
                                                              res.push_back(TRY(op(a[i], b[i])));

                                                          return res;
                                                      },
                                                      [&](double a, std::vector<double> b) -> Node::EvaluateData {
                                                          std::vector<double> res;
                                                          res.reserve(b.size());
                                                          for (double value : b)
                                                              res.push_back(TRY(op(a, value)));
                                                          return res;
                                                      },
                                                      [&](std::vector<double> a, double b) -> Node::EvaluateData {
                                                          std::vector<double> res;
                                                          res.reserve(a.size());
                                                          for (double value : a)
                                                              res.push_back(TRY(op(value, b)));
                                                          return res;
                                                      },
                                                      [&](double a, double b) -> Node::EvaluateData { return TRY(op(a, b)); },
                                                      [&](std::string a, std::string b) -> Node::EvaluateData {
                                                          if (operation != "+")
                                                              return std::unexpected(Node::EvaluationError{
                                                                  .description = "Invalid operation for strings",
                                                                  .input_errors = {"operation"},
                                                              });
                                                          return a + b;
                                                      },
                                                      [&](std::string a, std::vector<std::string> b) -> Node::EvaluateData {
                                                          if (operation != "+")
                                                              return std::unexpected(Node::EvaluationError{
                                                                  .description = "Invalid operation for strings",
                                                                  .input_errors = {"operation"},
                                                              });
                                                          std::vector<std::string> res;
                                                          res.reserve(b.size());
                                                          for (std::string value : b)
                                                              res.push_back(a + value);
                                                          return res;
                                                      },
                                                      [&](std::vector<std::string> a, std::string b) -> Node::EvaluateData {
                                                          if (operation != "+")
                                                              return std::unexpected(Node::EvaluationError{
                                                                  .description = "Invalid operation for strings",
                                                                  .input_errors = {"operation"},
                                                              });
                                                          std::vector<std::string> res;
                                                          res.reserve(a.size());
                                                          for (std::string value : a)
                                                              res.push_back(value + b);
                                                          return res;
                                                      },
                                                      [&](std::vector<std::string> a, std::vector<std::string> b) -> Node::EvaluateData {
                                                          if (operation != "+")
                                                              return std::unexpected(Node::EvaluationError{
                                                                  .description = "Invalid operation for strings",
                                                                  .input_errors = {"operation"},
                                                              });
                                                          if (a.size() != b.size())
                                                              return std::unexpected(Node::EvaluationError{
                                                                  .description = "Columns 'A' and 'B' are of different size",
                                                                  .input_errors = {"a", "b"},
                                                              });
                                                          std::vector<std::string> res;
                                                          res.reserve(a.size());
                                                          for (size_t i = 0; i < a.size(); i++)
                                                              res.push_back(a[i] + b[i]);
                                                          return res;
                                                      },
                                                      [](auto a, auto b) -> Node::EvaluateData {
                                                          return std::unexpected(Node::EvaluationError{
                                                              .description = "Invalid types for 'A' and 'B'",
                                                              .input_errors = {"a", "b"},
                                                          });
                                                      },
                                                  },
                                                  a_val,
                                                  b_val));

                return {};
            },
        });
}

#include "error.h"
#include "nodeplot.h"

using namespace NodePlot;

void register_binary_operation() {
    NodeRegistry::register_node("binary_operation",
                                Node{
                                    .type_id = "binary_operation",
                                    .display_name = "Binary Operation",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"operation", Node::Input{.id = "operation", .display_name = "Operation", .valid_data_types = {DataType::STRING}, .default_value = "+"}},
                                            {"a", Node::Input{.id = "a", .display_name = "A Value", .valid_data_types = {DataType::NUMBER, DataType::STRING, DataType::COLUMN}}},
                                            {"b", Node::Input{.id = "b", .display_name = "B Value", .valid_data_types = {DataType::NUMBER, DataType::STRING, DataType::COLUMN}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"result", Node::Output{.id = "result", .display_name = "Result", .valid_data_types = {DataType::NUMBER, DataType::STRING, DataType::COLUMN}}},
                                        };
                                    },
                                    .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                                        std::string operation = TRY(eng->get_input_value<std::string>(npf, node_id, "operation"));

                                        auto a_val = TRY(eng->get_input_value_variant<double, std::string, Column>(npf, node_id, "a"));
                                        auto b_val = TRY(eng->get_input_value_variant<double, std::string, Column>(npf, node_id, "b"));

                                        auto op = [&operation](auto a, auto b) -> ErrorOr<decltype(a)> {
                                            if (operation == "+")
                                                return a + b;
                                            if (operation == "-")
                                                return a - b;
                                            if (operation == "*")
                                                return a * b;
                                            if (operation == "/")
                                                return a / b;
                                            return ERR("Unknown operation");
                                        };

                                        cache.computed_outputs["result"] = TRY(std::visit(Utils::overloaded{
                                                                                              [&](Column a, Column b) -> ErrorOr<Data> {
                                                                                                  auto na = TRY_OR(a.as_numeric_column(), return ERR("Column 'A' is not numeric"));
                                                                                                  auto nb = TRY_OR(b.as_numeric_column(), return ERR("Column 'B' is not numeric"));

                                                                                                  if (na.values.size() != nb.values.size())
                                                                                                      return ERR("Columns 'A' and 'B' are of different size");

                                                                                                  Column::Numeric res;
                                                                                                  res.values.reserve(na.values.size());

                                                                                                  for (size_t i = 0; i < na.values.size(); i++)
                                                                                                      res.values.push_back(TRY(op(na.values[i], nb.values[i])));

                                                                                                  return Column{res};
                                                                                              },
                                                                                              [&](double a, Column b) -> ErrorOr<Data> {
                                                                                                  auto nb = TRY_OR(b.as_numeric_column(), return ERR("Column 'B' is not numeric"));

                                                                                                  Column::Numeric res;
                                                                                                  res.values.reserve(nb.values.size());

                                                                                                  for (double value : nb.values)
                                                                                                      res.values.push_back(TRY(op(a, value)));

                                                                                                  return Column{res};
                                                                                              },
                                                                                              [&](Column a, double b) -> ErrorOr<Data> {
                                                                                                  auto na = TRY_OR(a.as_numeric_column(), return ERR("Column 'A' is not numeric"));

                                                                                                  Column::Numeric res;
                                                                                                  res.values.reserve(na.values.size());

                                                                                                  for (double value : na.values)
                                                                                                      res.values.push_back(TRY(op(value, b)));

                                                                                                  return Column{res};
                                                                                              },
                                                                                              [&](double a, double b) -> ErrorOr<Data> { return TRY(op(a, b)); },
                                                                                              [&](std::string a, std::string b) -> ErrorOr<Data> { // TODO: Support string operations on columns
                                                                                                  if (operation != "+")
                                                                                                      return ERR("Invalid operation for strings");
                                                                                                  return a + b;
                                                                                              },
                                                                                              [](auto a, auto b) -> ErrorOr<Data> { return ERR("Invalid types for 'A' and 'B'"); },
                                                                                          },
                                                                                          a_val,
                                                                                          b_val));

                                        return {};
                                    },
                                });
}
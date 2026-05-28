#include "error.h"
#include "nodeplot.h"
#include "types.h"
#include <vector>

using namespace NodePlot;

void register_binary_operation() {
    NodeRegistry::register_node(
        "binary_operation",
        Node{
            .type_id = "binary_operation",
            .display_name = "Binary Operation",
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
            .evaluate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, EvaluatedNodeGraph::OutputCache& cache) -> ErrorOr<void> {
                std::string operation = TRY(eng->get_input_value<std::string>(npf, node_id, "operation"));

                auto a_val = TRY(eng->get_input_value_variant<double, std::string, std::vector<double>, std::vector<std::string>>(npf, node_id, "a"));
                auto b_val = TRY(eng->get_input_value_variant<double, std::string, std::vector<double>, std::vector<std::string>>(npf, node_id, "b"));

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
                                                                      [&](std::vector<double> a, std::vector<double> b) -> ErrorOr<Data> {
                                                                          if (a.size() != b.size())
                                                                              return ERR("Columns 'A' and 'B' are of different size");

                                                                          std::vector<double> res;
                                                                          res.reserve(a.size());

                                                                          for (size_t i = 0; i < a.size(); i++)
                                                                              res.push_back(TRY(op(a[i], b[i])));

                                                                          return res;
                                                                      },
                                                                      [&](double a, std::vector<double> b) -> ErrorOr<Data> {
                                                                          std::vector<double> res;
                                                                          res.reserve(b.size());
                                                                          for (double value : b)
                                                                              res.push_back(TRY(op(a, value)));
                                                                          return res;
                                                                      },
                                                                      [&](std::vector<double> a, double b) -> ErrorOr<Data> {
                                                                          std::vector<double> res;
                                                                          res.reserve(a.size());
                                                                          for (double value : a)
                                                                              res.push_back(TRY(op(value, b)));
                                                                          return res;
                                                                      },
                                                                      [&](double a, double b) -> ErrorOr<Data> { return TRY(op(a, b)); },
                                                                      [&](std::string a, std::string b) -> ErrorOr<Data> {
                                                                          if (operation != "+")
                                                                              return ERR("Invalid operation for strings");
                                                                          return a + b;
                                                                      },
                                                                      [&](std::string a, std::vector<std::string> b) -> ErrorOr<Data> {
                                                                          if (operation != "+")
                                                                              return ERR("Invalid operation for strings");
                                                                          std::vector<std::string> res;
                                                                          res.reserve(b.size());
                                                                          for (std::string value : b)
                                                                              res.push_back(a + value);
                                                                          return res;
                                                                      },
                                                                      [&](std::vector<std::string> a, std::string b) -> ErrorOr<Data> {
                                                                          if (operation != "+")
                                                                              return ERR("Invalid operation for strings");
                                                                          std::vector<std::string> res;
                                                                          res.reserve(a.size());
                                                                          for (std::string value : a)
                                                                              res.push_back(value + b);
                                                                          return res;
                                                                      },
                                                                      [](auto a, auto b) -> ErrorOr<Data> { return ERR("Invalid types for 'A' and 'B'"); },
                                                                  },
                                                                  a_val,
                                                                  b_val));

                return {};
            },
        });
}

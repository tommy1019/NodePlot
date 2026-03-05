#include "error.h"
#include "nodeplot.h"

template <typename T>
concept NumericType = std::integral<T> || std::floating_point<T>;

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const BinaryOperationNode& node, OutputIndex id) {
    auto a_val = TRY_OR(graph->get_output(node.i_a), return ERR("Failed to get value for 'A'"));
    auto b_val = TRY_OR(graph->get_output(node.i_b), return ERR("Failed to get value for 'B'"));
    auto operation = TRY_OR(graph->get_input(node.i_operation), return ERR("Could not get 'Operation' input"));

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

    return TRY(std::visit(overloaded{
                              [&](Column a, Column b) -> ErrorOr<NodeOutput> {
                                  auto na = TRY_OR(graph->column_as_numeric(a), return ERR("Column 'A' is not numeric"));
                                  auto nb = TRY_OR(graph->column_as_numeric(b), return ERR("Column 'B' is not numeric"));

                                  if (na.values.size() != nb.values.size())
                                      return ERR("Columns 'A' and 'B' are of different size");

                                  ColumnNumeric res;
                                  res.values.reserve(na.values.size());

                                  for (size_t i = 0; i < na.values.size(); i++)
                                      res.values.push_back(TRY(op(na.values[i], nb.values[i])));

                                  return Column{res};
                              },
                              [&](NumericType auto a, Column b) -> ErrorOr<NodeOutput> {
                                  auto nb = TRY_OR(graph->column_as_numeric(b), return ERR("Column 'B' is not numeric"));

                                  ColumnNumeric res;
                                  res.values.reserve(nb.values.size());

                                  for (size_t i = 0; i < nb.values.size(); i++)
                                      res.values.push_back(TRY(op(a, nb.values[i])));

                                  return Column{res};
                              },
                              [&](Column a, NumericType auto b) -> ErrorOr<NodeOutput> {
                                  auto na = TRY_OR(graph->column_as_numeric(a), return ERR("Column 'A' is not numeric"));

                                  ColumnNumeric res;
                                  res.values.reserve(na.values.size());

                                  for (size_t i = 0; i < na.values.size(); i++)
                                      res.values.push_back(TRY(op(na.values[i], b)));

                                  return Column{res};
                              },
                              [&](NumericType auto a, NumericType auto b) -> ErrorOr<NodeOutput> { return TRY(op(a, b)); },
                              [&](std::string a, std::string b) -> ErrorOr<NodeOutput> { // TODO: Support string operations on columns
                                  if (operation != "+")
                                      return ERR("Invalid operation for strings");
                                  return a + b;
                              },
                              [](auto a, auto b) -> ErrorOr<NodeOutput> { return ERR("Invalid types for 'A' and 'B'"); },
                          },
                          a_val,
                          b_val));
}

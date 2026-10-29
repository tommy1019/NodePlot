#pragma once

#include <filesystem>
#include <set>
#include <stack>
#include <variant>
#include <vector>

#include "error.h"
#include "node_graph.h"
#include "types.h"
#include "utils.h"

namespace NodePlot {

struct EvaluatedNodeGraph {
    GraphId graph_id;

    struct OutputCache {
        std::map<OutputId, Data> computed_outputs;
        std::optional<std::string> error;
    };

    std::map<NodeId, OutputCache> cache;

    ErrorOr<std::reference_wrapper<NodeGraph>> node_graph(NodePlotFile* npf);

    ErrorOr<Data> get_output_data(NodePlotFile* npf, NodeId node_id, OutputId output_id);

    ErrorOr<Data> get_input_data(NodePlotFile* npf, NodeId node_id, InputId input_id, bool fill_default_value);

    template <typename T>
    ErrorOr<T> try_data_type_conversion(Data data) {

        auto type_name = []<typename V>() -> std::string_view {
            std::string_view name, prefix, suffix;
#if defined(__clang__)
            name = __PRETTY_FUNCTION__;
            prefix = "std::string_view type_name() [V = ";
            suffix = "]";
#elif defined(__GNUC__)
            name = __PRETTY_FUNCTION__;
            prefix = "constexpr std::string_view type_name() [with V = ";
            suffix = "]";
#elif defined(_MSC_VER)
            name = __FUNCSIG__;
            prefix = "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl type_name<";
            suffix = ">(void)";
#endif

            name.remove_prefix(name.find(prefix) + prefix.size());
            name.remove_suffix(name.size() - name.find(suffix));
            return name;
        };

        return std::visit(Utils::overloaded{
                              [&](std::vector<std::string> str_vec) -> ErrorOr<T> {
                                  if constexpr (std::is_same_v<T, std::vector<std::string>>)
                                      return str_vec;
                                  if constexpr (std::is_same_v<T, std::vector<double>>) {
                                      std::vector<double> res;
                                      res.reserve(str_vec.size());
                                      for (auto& s : str_vec)
                                          res.push_back(Utils::string_to_double_with_nan(s));
                                      return res;
                                  }
                                  return ERR("Invalid type, cannot convert from string_column.");
                              },
                              [&](std::string v) -> ErrorOr<T> {
                                  if constexpr (std::is_same_v<T, std::string>)
                                      return v;
                                  if constexpr (std::is_same_v<T, std::filesystem::path>)
                                      return std::filesystem::path(v);
                                  return ERR("Invalid type, cannot convert from string. Value: '" + v + "'");
                              },
                              [&](auto v) -> ErrorOr<T> {
                                  if constexpr (std::is_same_v<decltype(v), T>)
                                      return v;

                                  std::string err = "Invalid type: No known conversion from '";
                                  err += type_name.template operator()<decltype(v)>();
                                  err += "' to '";
                                  err += type_name.template operator()<T>();
                                  err += "'";

                                  return ERR(err);
                              },
                          },
                          data);
    }

    template <typename... Ts>
    ErrorOr<std::variant<Ts...>> try_data_type_conversion_variant(Data data) {
        std::array<ErrorOr<std::variant<Ts...>>, sizeof...(Ts)> res;
        size_t i = 0;
        (
            [&]() {
                auto val = try_data_type_conversion<Ts>(data);
                if (val.has_value())
                    res[i++] = val.value();
                else
                    res[i++] = std::unexpected(val.error());
            }(),
            ...);

        for (size_t i = 0; i < sizeof...(Ts); i++)
            if (res[i].has_value())
                return res[i];

        return ERR("Invalid type");
    }

    template <typename T>
    ErrorOr<T> get_input_value(NodePlotFile* npf, NodeId node_id, InputId input_id, bool fill_default_value) {
        auto data = get_input_data(npf, node_id, input_id, fill_default_value);
        if (!data.has_value()) {
            return ERR(input_id + ": " + data.error());
        }

        if constexpr (requires { requires std::holds_alternative<T>(data); })
            if (std::holds_alternative<T>(data)) {
                return std::get<T>(data);
            }

        auto convert = try_data_type_conversion<T>(data.value());
        if (!convert.has_value()) {
            return ERR(input_id + ": " + convert.error());
        }

        return convert.value();
    }

    template <typename... Ts>
    ErrorOr<std::variant<Ts...>> get_input_value_variant(NodePlotFile* npf, NodeId node_id, InputId input_id, bool fill_default_value) {
        auto data = get_input_data(npf, node_id, input_id, fill_default_value);
        if (!data.has_value()) {
            return ERR(input_id + ": " + data.error());
        }

        auto convert = try_data_type_conversion_variant<Ts...>(data.value());
        if (!convert.has_value()) {
            return ERR(input_id + ": Invalid type");
        }

        return convert.value();
    }

    ErrorOr<PlotStyle> get_input_style(NodePlotFile* npf, NodeId node_id, InputId input_id);

    bool validate_data_type(Data& data, DataType type) {
        // TODO: Actually validate data type here
        return true;
    }
    bool validate_data_types(Data& data, std::vector<DataType> types) {
        bool success = false;
        for (auto& t : types)
            success |= validate_data_type(data, t);
        return success;
    }

    ErrorOr<void> node_inputs_changed(NodePlotFile* npf, NodeId id) {
        auto& ng = TRY(node_graph(npf)).get();

        std::set<NodeId> seen = {id};
        std::stack<NodeId> to_check;
        to_check.push(id);

        while (!to_check.empty()) {
            auto id = to_check.top();
            to_check.pop();

            cache[id] = {};

            for (auto [other_id, other_storage] : ng.nodes) {
                for (auto [input_id, value] : other_storage.input_storage) {
                    if (std::holds_alternative<InputPin>(value)) {
                        auto& pin = std::get<InputPin>(value);
                        if (pin.node_id == id) {
                            if (seen.insert(other_id).second) {
                                to_check.push(other_id);
                            };
                        }
                    }
                }
            }
        }

        return {};
    }
};

} // namespace NodePlot

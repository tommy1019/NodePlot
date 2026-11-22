#pragma once

#include <expected>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "error.h"
#include "node.h"
#include "types.h"

namespace NodePlot {
namespace Utils {

template <typename Map>
auto try_find(Map& map, auto key, std::string err) -> ErrorOr<std::reference_wrapper<typename Map::mapped_type>> {
    auto res = map.find(key);
    if (res == map.end())
        return ERR(err);
    return res->second;
}

template <typename Map>
auto try_find(const Map& map, auto key, std::string err) -> ErrorOr<typename Map::mapped_type> {
    auto res = map.find(key);
    if (res == map.end())
        return ERR(err);
    return res->second;
}

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

std::pair<double, double> find_limits(const std::vector<double>& res);

ErrorOr<double> string_to_double(std::string);
double string_to_double_with_nan(std::string);

template <typename Func, typename... Ts>
Node::EvaluateData vectorized(const Func& func, std::pair<InputId, std::variant<Ts, std::vector<Ts>>>... args) {
    bool is_column_output = !(std::holds_alternative<Ts>(args.second) && ...);

    using OutputType = decltype(std::invoke(func, std::get<Ts>(args.second)...));

    if (!is_column_output) {
        return Data(std::invoke(func, std::get<Ts>(args.second)...));
    } else {

        {
        }

        size_t column_size = std::max({std::holds_alternative<std::vector<Ts>>(args.second) ? std::get<std::vector<Ts>>(args.second).size() : 0 ...});

        if (column_size == 0) {
            std::set<InputId> cols;
            (
                [&](std::pair<InputId, std::variant<Ts, std::vector<Ts>>> arg) {
                    if (std::holds_alternative<std::vector<Ts>>(arg.second) && std::get<std::vector<Ts>>(arg.second).size() == 0)
                        cols.insert(arg.first);
                }(args),
                ...);
            return std::unexpected(Node::EvaluationError{
                .description = "One or more columns have size zero",
                .input_errors = cols,
            });
        }
        if (!(((std::holds_alternative<std::vector<Ts>>(args.second) ? (std::get<std::vector<Ts>>(args.second).size() == column_size) : true) && ...))) {
            std::set<InputId> cols;
            (
                [&](std::pair<InputId, std::variant<Ts, std::vector<Ts>>> arg) {
                    if (std::holds_alternative<std::vector<Ts>>(arg.second))
                        cols.insert(arg.first);
                }(args),
                ...);
            return std::unexpected(Node::EvaluationError{
                .description = "Column sizes do not match",
                .input_errors = cols,
            });
        }

        std::vector<OutputType> res;
        res.reserve(column_size);

        for (size_t i = 0; i < column_size; i++) {
            res.push_back(std::invoke(func, std::holds_alternative<std::vector<Ts>>(args.second) ? std::get<std::vector<Ts>>(args.second)[i] : std::get<Ts>(args.second)...));
        }

        return Data(res);
    }
}

} // namespace Utils
} // namespace NodePlot

#pragma once

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "error.h"
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
ErrorOr<Data> vectorized(const Func& func, std::variant<Ts, std::vector<Ts>>... args) {
    bool is_column_output = !(std::holds_alternative<Ts>(args) && ...);

    using OutputType = decltype(std::invoke(func, std::get<Ts>(args)...));

    if (!is_column_output) {
        return Data(std::invoke(func, std::get<Ts>(args)...));
    } else {
        size_t column_size = std::max({std::holds_alternative<std::vector<Ts>>(args) ? std::get<std::vector<Ts>>(args).size() : 0 ...});
        printf("Checking column size: %zu\n", column_size);
        if (column_size == 0)
            return ERR("Zero sized columns");
        if (!(((std::holds_alternative<std::vector<Ts>>(args) ? (std::get<std::vector<Ts>>(args).size() == column_size) : true) && ...)))
            return ERR("Columns sizes do not match");

        std::vector<OutputType> res;
        res.reserve(column_size);

        for (size_t i = 0; i < column_size; i++) {
            res.push_back(std::invoke(func, std::holds_alternative<std::vector<Ts>>(args) ? std::get<std::vector<Ts>>(args)[i] : std::get<Ts>(args)...));
        }

        return Data(res);
    }
}

} // namespace Utils
} // namespace NodePlot

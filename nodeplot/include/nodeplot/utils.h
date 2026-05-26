#pragma once

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "error.h"

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

} // namespace Utils
} // namespace NodePlot

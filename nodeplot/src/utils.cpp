#include "utils.h"

#include <charconv>

namespace NodePlot {
namespace Utils {

std::pair<double, double> find_limits(const std::vector<double>& res) {
    double low = std::numeric_limits<double>::infinity();
    double high = -std::numeric_limits<double>::infinity();
    for (auto& v : res) {
        low = std::min(low, v);
        high = std::max(high, v);
    }
    return {low, high};
}

ErrorOr<double> string_to_double(std::string s) {
    double value;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec == std::errc()) {
        return value;
    } else if (ec == std::errc::invalid_argument) {
        return ERR("Invalid number");
    } else if (ec == std::errc::result_out_of_range) {
        return ERR("Number out of range");
    } else {
        return ERR("Error converting string to number");
    }
}

double string_to_double_with_nan(std::string s) {
    double value;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec == std::errc()) {
        return value;
    } else {
        return std::numeric_limits<float>::quiet_NaN();
    }
}
} // namespace Utils
} // namespace NodePlot

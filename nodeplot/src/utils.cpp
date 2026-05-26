#include "utils.h"

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

} // namespace Utils
} // namespace NodePlot

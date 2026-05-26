#include "types.h"

#include <print>

#include <sys/mman.h>

#include "error.h"

namespace NodePlot {

MappedFile::~MappedFile() {
    if (munmap(data, file_size) == -1) {
        REQUIRE_NOT_REACHED("Failed to unmap CSV file");
    }
}

Pos FigureBounds::normalize(Pos p) const {
    if (x_axis_log_scale)
        p.x = std::log10(p.x);
    if (y_axis_log_scale)
        p.y = std::log10(p.y);

    p.x = (p.x + x_transform_pre) * x_scale + x_transform_post;
    p.y = (p.y + y_transform_pre) * y_scale + y_transform_post;

    return p;
}

} // namespace NodePlot

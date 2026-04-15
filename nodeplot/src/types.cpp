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

} // namespace NodePlot
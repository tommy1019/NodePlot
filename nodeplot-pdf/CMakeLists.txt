cmake_minimum_required(VERSION 3.25)

set(SOURCES
    src/main.cpp
)

include(FetchContent)
FetchContent_Declare(
    libharu
    GIT_REPOSITORY https://github.com/libharu/libharu
    GIT_TAG v2.4.6
)
FetchContent_MakeAvailable(libharu)

add_executable(nodeplot-pdf ${SOURCES})

target_link_libraries(nodeplot-pdf PUBLIC nodeplot hpdf)
target_include_directories(nodeplot-pdf PRIVATE ${libharu_SOURCE_DIR}/include ${libharu_BINARY_DIR}/include)

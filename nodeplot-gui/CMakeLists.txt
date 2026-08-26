cmake_minimum_required(VERSION 3.25)

set(SOURCES
    src/main.cpp
    src/window.cpp
    src/svg_renderer.cpp
    src/node_renderer.cpp
)

find_package(SDL2 REQUIRED CONFIG)
find_package(OpenGL REQUIRED)

FetchContent_Declare(
    imgui_src
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui_src)
add_library(imgui STATIC
    ${imgui_src_SOURCE_DIR}/imgui.cpp
    ${imgui_src_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_src_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_src_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_src_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_src_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imgui_src_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_src_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_src_SOURCE_DIR}
    ${imgui_src_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC SDL2::SDL2 ${OPENGL_LIBRARIES})

FetchContent_Declare(json 
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

include(FetchContent)
FetchContent_Declare(
    lunasvg
    GIT_REPOSITORY https://github.com/sammycage/lunasvg.git
    GIT_TAG master
)
FetchContent_MakeAvailable(lunasvg)

FetchContent_Declare(
    nativefiledialog
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
    GIT_TAG        v1.3.0
)
FetchContent_MakeAvailable(nativefiledialog)

add_executable(nodeplot-gui ${SOURCES})

target_link_libraries(nodeplot-gui PUBLIC nodeplot lunasvg::lunasvg imgui nfd)

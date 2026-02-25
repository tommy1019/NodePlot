cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(nodeplot VERSION 1.0 LANGUAGES CXX)

include(FetchContent)

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
  portable_file_dialogs
  GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs
  GIT_TAG        main
)
FetchContent_MakeAvailable(portable_file_dialogs)

add_subdirectory(nodeplot)
add_subdirectory(nodeplot-gui)
add_subdirectory(nodeplot-cmd)
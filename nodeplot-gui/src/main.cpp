#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>

#include <imgui.h>

#include <portable-file-dialogs.h>

#include <nodeplot/error.h>
#include <nodeplot/nodeplot.h>

#include "node_renderer.h"
#include "nodeplot/node_graph.h"
#include "nodeplot/types.h"
#include "nodeplot/utils.h"
#include "svg_renderer.h"
#include "window.h"

int main(int argc, char** argv) {

    NodePlot::NodeRegistry::init();

    // TODO: Check for infinite loops

    NodePlot::NodePlotFile npf = [&]() {
        if (argc == 3) {
            if (strcmp("--new", argv[1]) == 0) {
                return MUST(NodePlot::NodePlotFile::create(argv[2]));
            } else {
                REQUIRE_NOT_REACHED("Too many arguments")
            }
        } else {
            std::ifstream ifs(argv[1]);
            if (!ifs)
                REQUIRE_NOT_REACHED("Could not open input file: '{}'", argv[1]);
            return MUST(NodePlot::NodePlotFile::from_json(nlohmann::json::parse(ifs), argv[1]));
        }
    }();

    auto save = [&](std::optional<std::filesystem::path> filename = {}) {
        std::ofstream ofs(filename.value_or(npf.path));
        if (!ofs)
            REQUIRE_NOT_REACHED("Failed to open file for writing");

        ofs << MUST(npf.to_json()).dump(4);
    };

    MUST(Window::global_init());
    auto window = MUST(Window::create({
        .width = 1600,
        .height = 1200,
    }));

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameBorderSize = 1.0f;

    auto svg_renderer = MUST(SVGRenderer::create({-1, -1}, R"SVG(<svg width="400" height="400" xmlns="http://www.w3.org/2000/svg"></svg>)SVG"));

    NodePlot::GraphId current_graph = "main";

    auto cur_ng = [&]() -> NodePlot::NodeGraph& {
        auto cur = npf.graphs.find(current_graph);
        if (cur == npf.graphs.end()) {
            REQUIRE_NOT_REACHED("Invalid current graph");
        }
        return cur->second;
    };

    auto create_eng = [&](NodePlot::GraphId gid) -> NodePlot::EvaluatedNodeGraph {
        auto cur = npf.graphs.find(gid);
        if (cur == npf.graphs.end()) {
            REQUIRE_NOT_REACHED("Invalid current graph");
        }
        return NodePlot::EvaluatedNodeGraph{.graph_id = current_graph};
    };

    NodePlot::EvaluatedNodeGraph main_eng = create_eng(current_graph);
    NodePlot::EvaluatedNodeGraph sub_eng = create_eng(current_graph);
    auto cur_eng = [&]() -> NodePlot::EvaluatedNodeGraph& {
        if (current_graph == "main")
            return main_eng;
        return sub_eng;
    };

    NodeRenderer* node_renderer;
    auto set_current_graph = [&](NodePlot::GraphId new_id) {
        current_graph = new_id;
        if (current_graph != "main") {
            sub_eng = create_eng(current_graph);
            // This is a hacky way to keep this callback around, there should be a better way to design this callback
            auto old = node_renderer;
            node_renderer = new NodeRenderer(&npf, &sub_eng);
            node_renderer->set_current_graph_id = old->set_current_graph_id;
        } else {
            auto old = node_renderer;
            node_renderer = new NodeRenderer(&npf, &main_eng);
            node_renderer->set_current_graph_id = old->set_current_graph_id;
        }
    };
    node_renderer = new NodeRenderer(&npf, &main_eng);
    node_renderer->set_current_graph_id = set_current_graph;

    auto update_svg = [&]() {
        for (auto& n : MUST(main_eng.node_graph(&npf)).get().nodes) {
            if (n.second.type_id == "output") {
                auto svg_or_err = main_eng.get_output_data(&npf, n.first, "svg");
                if (!svg_or_err.has_value())
                    continue;
                if (!std::holds_alternative<std::string>(svg_or_err.value()))
                    REQUIRE_NOT_REACHED("SVG is not a string type");
                MAYBE(svg_renderer->set_svg(std::get<std::string>(svg_or_err.value())));
            }
        }
    };
    update_svg();

    std::optional<ImVec2> left_drag_start = std::nullopt;
    std::set<NodePlot::NodeId> dragged_windows = {};
    std::set<NodePlot::NodeId> selected_windows;

    MUST(window->event_loop(NodePlot::Utils::overloaded{[&](Window::RenderEvent) -> ErrorOr<void> {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("My Fullscreen Window",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
                         | ImGuiWindowFlags_NoBackground);
        ImGui::PopStyleVar();

        if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::GetIO().KeyCtrl) {
            save();
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    save();
                }
                if (ImGui::MenuItem("Save-As")) {
                    auto selection = pfd::save_file("Save Nodeplot File As", "", {"Json File", "*.json"}, pfd::opt::none).result();
                    if (!selection.empty()) {
                        std::string filename = selection;
                        if (!filename.ends_with(".json"))
                            filename += ".json";

                        save(filename);
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("NodeGraph")) {

                if (ImGui::BeginMenu("Edit")) {
                    for (auto [id, graph] : npf.graphs) {
                        if (ImGui::MenuItem(id.c_str())) {
                            set_current_graph(id);
                        }
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Add")) {
                    for (auto n : NodePlot::NodeRegistry::node_map) {
                        if (ImGui::MenuItem(n.second.display_name.c_str())) {
                            auto win_size = ImGui::GetWindowSize();
                            auto placement_pos = node_renderer->screen_to_world(ImVec2(win_size.x, win_size.y));
                            cur_ng().create_node(&npf, current_graph, n.second.type_id, placement_pos.x, placement_pos.y);
                        }
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::BeginChild("NodeEditor",
                          ImVec2(ImGui::GetContentRegionAvail().x * 0.75, 0),
                          ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_None | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            auto& nodes = cur_ng().nodes;

            auto render_node_editor_position = ImGui::GetCursorScreenPos();
            auto render_node_editor_size = ImGui::GetWindowSize();

            ImVec2 clip_start = {
                render_node_editor_position.x - ImGui::GetStyle().WindowPadding.x,
                render_node_editor_position.y - ImGui::GetStyle().WindowPadding.y,
            };
            ImVec2 clip_end = {
                render_node_editor_position.x - ImGui::GetStyle().WindowPadding.x + render_node_editor_size.x,
                render_node_editor_position.y - ImGui::GetStyle().WindowPadding.y + render_node_editor_size.y,
            };
            ImGui::GetForegroundDrawList()->PushClipRect(clip_start, clip_end, true);
            ImGui::GetBackgroundDrawList()->PushClipRect(clip_start, clip_end, true);

            // Grid Lines
            if (true) {
                constexpr float GRID_COLOR = 0.85f;
                auto color = ImGui::GetColorU32(ImVec4(GRID_COLOR, GRID_COLOR, GRID_COLOR, 1.0f));

                auto render_list = ImGui::GetBackgroundDrawList();

                ImVec2 grid_size = ImVec2(100, 100);

                ImVec2 grid_size_screen = ImVec2(node_renderer->world_to_screen(grid_size).x - node_renderer->world_to_screen(ImVec2(0, 0)).x,
                                                 node_renderer->world_to_screen(grid_size).y - node_renderer->world_to_screen(ImVec2(0, 0)).y);

                ImVec2 start = node_renderer->screen_to_world(render_node_editor_position);
                ImVec2 end = node_renderer->screen_to_world(render_node_editor_size);

                start.x = std::floor(start.x / 100.0f) * 100.0f;
                start.y = std::floor(start.y / 100.0f) * 100.0f;

                end.x = std::ceil(end.x / 100.0f) * 100.0f;
                end.y = std::ceil(end.y / 100.0f) * 100.0f;

                start = node_renderer->world_to_screen(start);
                end = node_renderer->world_to_screen(end);

                for (float x = start.x; x <= end.x; x += grid_size_screen.x) {
                    render_list->AddLine({x, start.y}, {x, end.y}, color);
                }

                for (float y = start.y; y <= end.y; y += grid_size_screen.y) {
                    render_list->AddLine({start.x, y}, {end.x, y}, color);
                }
            }

            for (auto& [id, storage] : nodes) {
                node_renderer->render_input_paths(id, storage);
            }

            auto old_style = ImGui::GetStyle();
            ImGui::GetStyle().ScaleAllSizes(node_renderer->scene_scale);
            ImGui::PushFont(nullptr, node_renderer->scene_scale * 13);
            ImGui::GetStyle().SeparatorSize = 1.0f;

            std::set<NodePlot::NodeId> nodes_to_delete;

            bool did_update = false;

            if (left_drag_start.has_value()) {
                if (dragged_windows.empty()) {
                    auto cur_mouse = ImGui::GetIO().MousePos;
                    ImGui::GetForegroundDrawList()->AddRect(
                        {
                            std::min(cur_mouse.x, left_drag_start->x),
                            std::min(cur_mouse.y, left_drag_start->y),
                        },
                        {
                            std::max(cur_mouse.x, left_drag_start->x),
                            std::max(cur_mouse.y, left_drag_start->y),
                        },
                        ImColor(150, 150, 150, 255));
                } else {
                    for (auto& id : dragged_windows) {
                        MAYBE(NodePlot::Utils::try_find(nodes, id, "Invalid dragged window").and_then([&](NodePlot::NodeGraph::NodeStorage& storage) -> ErrorOr<void> {
                            storage.pos.x += io.MouseDelta.x / node_renderer->scene_scale;
                            storage.pos.y += io.MouseDelta.y / node_renderer->scene_scale;
                            return {};
                        }));
                    }
                }

                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    if (dragged_windows.empty()) {
                        if (!ImGui::GetIO().KeyShift)
                            selected_windows.clear();
                        auto cur_mouse = ImGui::GetIO().MousePos;
                        for (auto& [id, storage] : nodes) {
                            auto screen_pos = node_renderer->world_to_screen({storage.pos.x, storage.pos.y});
                            if (std::min(cur_mouse.x, left_drag_start->x) < screen_pos.x && screen_pos.x < std::max(cur_mouse.x, left_drag_start->x)
                                && std::min(cur_mouse.y, left_drag_start->y) < screen_pos.y && screen_pos.y < std::max(cur_mouse.y, left_drag_start->y)) {
                                selected_windows.insert(id);
                            }
                        }
                    } else {
                        dragged_windows.clear();
                    }
                    left_drag_start = std::nullopt;
                }
            }

            for (auto& [id, storage] : nodes) {
                auto out_cache = cur_eng().cache[id];

                if (out_cache.error.has_value()) {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.7f, 0.7f, 1.0f));
                }

                if (selected_windows.contains(id)) {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.9f, 0.9f, 1.0f, 1.0f));
                }

                ImGui::SetNextWindowPos(node_renderer->world_to_screen(ImVec2(storage.pos.x, storage.pos.y)), ImGuiCond_Always);
                if (ImGui::BeginChild(std::to_string(id).c_str(),
                                      {0, 0},
                                      ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY,
                                      ImGuiWindowFlags_None | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                    if (MUST(node_renderer->render_node(id, storage))) {
                        did_update = true;
                        MUST(cur_eng().node_inputs_changed(&npf, id));

                        if (current_graph != "main") {
                            auto& ng = npf.graphs.at("main");
                            for (auto n : ng.nodes) {
                                if (n.second.type_id == "function") {
                                    MUST(main_eng.node_inputs_changed(&npf, n.first));
                                }
                            }
                        }
                    }

                    if (out_cache.error.has_value()) {
                        ImGui::Separator();
                        ImGui::Text("%s", out_cache.error.value().c_str());
                    }

                    if (!left_drag_start.has_value() && ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                        left_drag_start = ImGui::GetIO().MousePos;
                        if (selected_windows.empty()) {
                            dragged_windows.clear();
                            dragged_windows.insert(id);
                        } else {
                            dragged_windows = selected_windows;
                        }
                    }

                    if (ImGui::BeginPopupContextWindow()) {
                        if (ImGui::MenuItem("Delete")) {
                            nodes_to_delete.insert(id);
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::EndChild();

                if (selected_windows.contains(id)) {
                    ImGui::PopStyleColor();
                }

                if (out_cache.error.has_value()) {
                    ImGui::PopStyleColor();
                }
            }

            if (nodes_to_delete.size() > 0) {
                did_update = true;

                if (current_graph == "main") {
                    for (auto& n : nodes_to_delete) {
                        MAYBE(main_eng.node_inputs_changed(&npf, n));
                    }
                } else {
                    auto& ng = npf.graphs.at("main");
                    for (auto n : ng.nodes) {
                        if (n.second.type_id == "function") {
                            MUST(main_eng.node_inputs_changed(&npf, n.first));
                        }
                    }
                }

                for (auto& n : nodes_to_delete) {
                    nodes.erase(n);
                }
            }

            if (did_update) {
                update_svg();
            }

            if (ImGui::IsWindowHovered()) {

                if (!left_drag_start.has_value() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    left_drag_start = ImGui::GetIO().MousePos;
                }

                if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                    node_renderer->scene_translation.x -= io.MouseDelta.x / node_renderer->scene_scale;
                    node_renderer->scene_translation.y -= io.MouseDelta.y / node_renderer->scene_scale;
                }

                if (ImGui::GetIO().KeyShift) {
                    if (ImGui::GetIO().MouseWheel != 0.0 || ImGui::GetIO().MouseWheelH != 0.0) {
                        ImVec2 mouse_pos = ImVec2(ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x, ImGui::GetIO().MousePos.y - ImGui::GetWindowPos().y);

                        auto pre = node_renderer->screen_to_world(mouse_pos);

                        float scroll = ImGui::GetIO().MouseWheel + ImGui::GetIO().MouseWheelH;
                        float scale_delta = (scroll > 0.0f) ? 1.1f : 0.9f;
                        node_renderer->scene_scale *= scale_delta;

                        auto post = node_renderer->screen_to_world(mouse_pos);

                        node_renderer->scene_translation.x += pre.x - post.x;
                        node_renderer->scene_translation.y += pre.y - post.y;
                    }
                } else {
                    constexpr float SCROLL_SPEED = 8;
                    if (ImGui::GetIO().MouseWheel != 0.0) {
                        node_renderer->scene_translation.y -= ImGui::GetIO().MouseWheel * SCROLL_SPEED / node_renderer->scene_scale;
                    }
                    if (ImGui::GetIO().MouseWheelH != 0.0) {
                        node_renderer->scene_translation.x -= ImGui::GetIO().MouseWheelH * SCROLL_SPEED / node_renderer->scene_scale;
                    }
                }
            }

            ImGui::GetStyle() = old_style;
            ImGui::PopFont();

            ImGui::GetForegroundDrawList()->PopClipRect();
            ImGui::GetBackgroundDrawList()->PopClipRect();
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("PlotViewer", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);
        {
            MAYBE(svg_renderer->set_size({ImGui::GetWindowSize().x, ImGui::GetWindowSize().y}));
            svg_renderer->draw();
        }
        ImGui::EndChild();

        ImGui::End();

        return {};
    }}));

    window.reset();
    MUST(Window::global_deinit());
}

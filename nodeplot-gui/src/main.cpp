#include <fstream>
#include <memory>
#include <string>
#include <variant>

#include <imgui.h>

#include <nodeplot/error.h>
#include <nodeplot/nodeplot.h>

#include <portable-file-dialogs.h>

#include "render_node.h"
#include "render_node_graph.h"
#include "style.h"
#include "svg_renderer.h"
#include "window.h"

auto SideBySide = [](auto A_id, auto A, auto B_id, auto B, float inital_seperation = 0.5f) {
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::BeginChild(A_id,
                      ImVec2(ImGui::GetContentRegionAvail().x * inital_seperation, 0),
                      ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_None | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    A();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild(B_id, ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);
    B();
    ImGui::EndChild();
};

int main(int argc, char** argv) {

    // TODO: Check for infinite loops

    RenderNodeGraph render_node_graph = [&]() {
        if (argc == 2) {
            if (strcmp("--new", argv[1]) == 0) {
                return RenderNodeGraph {
                .eval_node_graph = EvaluatedNodeGraph{
                    .node_graph = MUST(NodeGraph::create(argv[1])),
                },
            };
            }
            return RenderNodeGraph {
                .eval_node_graph = EvaluatedNodeGraph{
                    .node_graph = MUST(NodeGraph::read(argv[1])),
                },
            };
        }

        return RenderNodeGraph{
            .eval_node_graph = EvaluatedNodeGraph{
                .node_graph = MUST(NodeGraph::create("nodeplot.json")),
            },
        };
    }();

    MUST_DETAILED(Window::global_init());
    auto window = MUST_DETAILED(Window::create({
        .width = 1600,
        .height = 1200,
    }));

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameBorderSize = 1.0f;

    render_node_graph.svg_renderer = MUST(SVGRenderer::create({-1, -1}, R"SVG(<svg width="400" height="400" xmlns="http://www.w3.org/2000/svg"></svg>)SVG"));
    render_node_graph.update_target();

    window->event_loop(overloaded{[&](Window::RenderEvent) -> ErrorOr<void> {
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
            render_node_graph.eval_node_graph.node_graph.save();
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    render_node_graph.eval_node_graph.node_graph.save();
                }
                if (ImGui::MenuItem("Save-As")) {
                    auto selection = pfd::save_file("Save Nodeplot File As", "", {"Json File", "*.json"}, pfd::opt::none).result();
                    if (!selection.empty()) {
                        std::string filename = selection;
                        if (!filename.ends_with(".json"))
                            filename += ".json";
                        render_node_graph.eval_node_graph.node_graph.save(filename);
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("NodeGraph")) {
                if (ImGui::BeginMenu("Add")) {
                    for_each_type<Node>([&]<typename T>() {
                        if (ImGui::MenuItem(T::name().c_str())) {
                            auto win_size = ImGui::GetWindowSize();
                            auto placement_pos = render_node_graph.screen_to_world(ImVec2(win_size.x, win_size.y));

                            T new_node{};
                            new_node.pos = {placement_pos.x, placement_pos.y};
                            render_node_graph.eval_node_graph.node_graph.add_node(new_node);
                        }
                    });
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        SideBySide(
            "NodeEditor",
            [&]() {
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

                    ImVec2 grid_size_screen = ImVec2(render_node_graph.world_to_screen(grid_size).x - render_node_graph.world_to_screen(ImVec2(0, 0)).x,
                                                     render_node_graph.world_to_screen(grid_size).y - render_node_graph.world_to_screen(ImVec2(0, 0)).y);

                    ImVec2 start = render_node_graph.screen_to_world(render_node_editor_position);
                    ImVec2 end = render_node_graph.screen_to_world(render_node_editor_size);

                    start.x = std::floor(start.x / 100.0f) * 100.0f;
                    start.y = std::floor(start.y / 100.0f) * 100.0f;

                    end.x = std::ceil(end.x / 100.0f) * 100.0f;
                    end.y = std::ceil(end.y / 100.0f) * 100.0f;

                    start = render_node_graph.world_to_screen(start);
                    end = render_node_graph.world_to_screen(end);

                    for (float x = start.x; x <= end.x; x += grid_size_screen.x) {
                        render_list->AddLine({x, start.y}, {x, end.y}, color);
                    }

                    for (float y = start.y; y <= end.y; y += grid_size_screen.y) {
                        render_list->AddLine({start.x, y}, {end.x, y}, color);
                    }
                }

                // Draw node connection lines
                {
                    for (auto& id_node : render_node_graph.eval_node_graph.node_graph.nodes()) {
                        std::visit(
                            [&](auto& node) {
                                auto window_pos = render_node_graph.world_to_screen(ImVec2(node.pos.first, node.pos.second));

                                size_t y_pos = 0;
                                y_pos += NODE_ELEMENT_HEIGHT / 2;

                                auto render_pin = [&]<typename T>(InputPin<T> pin) {
                                    auto other_node = render_node_graph.eval_node_graph.node_graph.nodes().find(pin.node);
                                    if (other_node == render_node_graph.eval_node_graph.node_graph.nodes().end())
                                        return;

                                    auto other_node_pos = render_node_graph.world_to_screen(std::visit([](auto node) { return ImVec2{node.pos.first, node.pos.second}; }, other_node->second));

                                    auto pin_offset = std::visit(
                                        [&](auto node) {
                                            size_t height = render_node_graph.get_header_height(node) + render_node_graph.get_input_height(node);
                                            bool found = false;
                                            for_each_tuple(
                                                [&](auto output) {
                                                    if (!found)
                                                        height += NODE_ELEMENT_HEIGHT;
                                                    if (std::get<NODE_OUTPUT_INDEX_ID>(output) == pin.output_index) {
                                                        found = true;
                                                    }
                                                },
                                                node.outputs());
                                            return height;
                                        },
                                        other_node->second);

                                    render_node_graph.draw_node_path(
                                        {
                                            other_node_pos.x + (NODE_WIDTH - NODE_LEFT_PADDING) * render_node_graph.scene_scale,
                                            other_node_pos.y + (pin_offset + NODE_IO_CIRCLE_RADIUS) * render_node_graph.scene_scale,
                                        },
                                        {
                                            window_pos.x + (NODE_LEFT_PADDING + NODE_IO_CIRCLE_RADIUS) * render_node_graph.scene_scale,
                                            window_pos.y + (y_pos + NODE_IO_CIRCLE_RADIUS) * render_node_graph.scene_scale,
                                        });
                                };

                                for_each_tuple(
                                    [&](auto input_tuple) {
                                        overloaded{
                                            [&]<typename T>(std::vector<Input<T>>& v) {
                                                y_pos += NODE_ELEMENT_HEIGHT;
                                                for (auto& a : v) {
                                                    y_pos += NODE_ELEMENT_HEIGHT;
                                                    if (std::holds_alternative<InputPin<T>>(a))
                                                        render_pin(std::get<InputPin<T>>(a));
                                                }
                                            },
                                            [&]<typename T>(std::vector<InputPin<T>>& v) {
                                                y_pos += NODE_ELEMENT_HEIGHT;
                                                for (auto& a : v) {
                                                    y_pos += NODE_ELEMENT_HEIGHT;
                                                    render_pin(a);
                                                }
                                            },
                                            [&]<typename T>(Input<T>& v) {
                                                y_pos += NODE_ELEMENT_HEIGHT;
                                                if (std::holds_alternative<InputPin<T>>(v))
                                                    render_pin(std::get<InputPin<T>>(v));
                                            },
                                            [&]<typename T>(InputPin<T>& v) {
                                                y_pos += NODE_ELEMENT_HEIGHT;
                                                render_pin(v);
                                            },
                                            [&](auto&) { y_pos += NODE_ELEMENT_HEIGHT; },
                                        }(std::get<NODE_INPUT_INDEX_STORAGE>(input_tuple));
                                    },
                                    node.inputs());
                            },
                            id_node.second);
                    }
                }

                auto old_style = ImGui::GetStyle();
                ImGui::GetStyle().ScaleAllSizes(render_node_graph.scene_scale);
                ImGui::PushFont(NULL, render_node_graph.scene_scale * 13);
                ImGui::GetStyle().SeparatorSize = 1.0f;

                std::set<NodeId> nodes_to_delete;

                for (auto& id_node : render_node_graph.eval_node_graph.node_graph.nodes()) {
                    std::visit(
                        [&](auto& node) {
                            EvaluatedNodeGraph::LoadedNode& eval_node = render_node_graph.eval_node_graph.loaded_nodes[node.id];

                            auto node_error = eval_node.error_message;

                            if (node_error.has_value())
                                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.7f, 0.7f, 1.0f));

                            ImGui::SetNextWindowPos(render_node_graph.world_to_screen(ImVec2(node.pos.first, node.pos.second)), ImGuiCond_Always);

                            size_t node_height = 0;
                            {
                                node_height += render_node_graph.get_header_height(node);
                                node_height += render_node_graph.get_input_height(node);
                                node_height += render_node_graph.get_output_height(node);
                                if (node_error.has_value()) {
                                    node_height += NODE_ELEMENT_HEIGHT;
                                    node_height += NODE_ELEMENT_HEIGHT / 2;
                                }
                                node_height += NODE_ELEMENT_HEIGHT;
                            }

                            ImGui::SetNextWindowSize({NODE_WIDTH * render_node_graph.scene_scale, node_height * render_node_graph.scene_scale});
                            if (ImGui::BeginChild(std::to_string(node.id).c_str(),
                                                  {0, 0},
                                                  ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY,
                                                  ImGuiWindowFlags_None | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

                                ImGui::Dummy({300 * render_node_graph.scene_scale, node_height * render_node_graph.scene_scale});

                                if (ImGui::IsWindowHovered()) {
                                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                                        node.pos.first += io.MouseDelta.x / render_node_graph.scene_scale;
                                        node.pos.second += io.MouseDelta.y / render_node_graph.scene_scale;
                                    }
                                }

                                node_render(&render_node_graph, node);

                                if (node_error.has_value()) {
                                    ImGui::SetCursorPos(
                                        {NODE_LEFT_PADDING * render_node_graph.scene_scale, (node_height - NODE_ELEMENT_HEIGHT - NODE_ELEMENT_HEIGHT / 2) * render_node_graph.scene_scale});
                                    ImGui::Separator();
                                    ImGui::SetCursorPos({NODE_LEFT_PADDING * render_node_graph.scene_scale, (node_height - NODE_ELEMENT_HEIGHT) * render_node_graph.scene_scale});
                                    ImGui::Text("%s", node_error.value().c_str());
                                }

                                // ImGui::Separator();
                                // ImGui::Text("Node: %lld", node.id);

                                if (ImGui::BeginPopupContextWindow()) {
                                    if (ImGui::MenuItem("Delete")) {
                                        nodes_to_delete.insert(node.id);
                                        render_node_graph.update_target(node.id);
                                    }

                                    if (std::is_same_v<decltype(node), OutputNode&>) {
                                        if (ImGui::MenuItem("Visualize Output")) {
                                            render_node_graph.eval_node_graph.node_graph.visualize_node() = node.id;
                                            render_node_graph.update_target(node.id);
                                        }
                                    }

                                    ImGui::EndPopup();
                                }
                            }
                            ImGui::EndChild();

                            if (node_error.has_value())
                                ImGui::PopStyleColor();
                        },
                        id_node.second);
                }

                if (nodes_to_delete.size() > 0) {
                    for (auto& n : nodes_to_delete)
                        render_node_graph.eval_node_graph.node_graph.nodes().erase(n);
                    render_node_graph.update_target();
                }

                if (ImGui::IsWindowHovered()) {
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                        render_node_graph.scene_translation.x -= io.MouseDelta.x / render_node_graph.scene_scale;
                        render_node_graph.scene_translation.y -= io.MouseDelta.y / render_node_graph.scene_scale;
                    }
                }

                ImGui::GetStyle() = old_style;
                ImGui::PopFont();

                if (ImGui::GetIO().MouseWheel != 0.0) {

                    ImVec2 mouse_pos = ImVec2(ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x, ImGui::GetIO().MousePos.y - ImGui::GetWindowPos().y);

                    auto pre = render_node_graph.screen_to_world(mouse_pos);

                    float scale_delta = (ImGui::GetIO().MouseWheel > 0.0f) ? 1.1f : 0.9f;
                    render_node_graph.scene_scale *= scale_delta;

                    auto post = render_node_graph.screen_to_world(mouse_pos);

                    render_node_graph.scene_translation.x += pre.x - post.x;
                    render_node_graph.scene_translation.y += pre.y - post.y;
                }

                ImGui::GetForegroundDrawList()->PopClipRect();
                ImGui::GetBackgroundDrawList()->PopClipRect();
            },
            "PlotViewer",
            [&]() {
                MAYBE(render_node_graph.svg_renderer->set_size({ImGui::GetWindowSize().x, ImGui::GetWindowSize().y}));
                render_node_graph.svg_renderer->draw();
            },
            0.75f);

        // ImGui::ShowDemoWindow();

        ImGui::End();

        return {};
    }});

    window.reset();
    MUST_DETAILED(Window::global_deinit());
}

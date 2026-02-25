#include <fstream>
#include <memory>
#include <string>
#include <variant>

#include <imgui.h>

#include <nodeplot/error.h>
#include <nodeplot/nodeplot.h>

#include "imgui_internal.h"
#include "portable-file-dialogs.h"
#include "render_node.h"
#include "render_node_graph.h"
#include "svg_renderer.h"
#include "window.h"

auto SideBySide = [](auto A_id, auto A, auto B_id, auto B, float inital_seperation = 0.5f) {
    ImGui::BeginChild(A_id, ImVec2(ImGui::GetContentRegionAvail().x * inital_seperation, 0), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
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
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
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
                        if (ImGui::MenuItem(T::type().c_str())) {
                            auto win_size = ImGui::GetWindowSize();

                            T new_node{};
                            new_node.pos = {
                                -render_node_graph.scene_translation.x + win_size.x / 2,
                                -render_node_graph.scene_translation.y + win_size.y / 2,
                            };
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

                ImGui::GetForegroundDrawList()->PushClipRect(render_node_editor_position,
                                                             {
                                                                 render_node_editor_position.x + render_node_editor_size.x - ImGui::GetStyle().WindowPadding.x,
                                                                 render_node_editor_position.y + render_node_editor_size.y,
                                                             },
                                                             true);

                // Grid Lines
                if (true) {
                    auto color = ImGui::GetColorU32(ImVec4(0.85f, 0.85f, 0.85f, 1.0f));

                    auto render_list = ImGui::GetWindowDrawList();

                    constexpr float GRID_X = 100.0f;
                    constexpr float GRID_Y = 100.0f;

                    float x_offset = render_node_graph.scene_translation.x - std::floor(render_node_graph.scene_translation.x / GRID_X) * GRID_X;
                    float y_offset = render_node_graph.scene_translation.y - std::floor(render_node_graph.scene_translation.y / GRID_Y) * GRID_Y;

                    for (float x = 0; x < render_node_editor_size.x; x += GRID_X) {
                        render_list->AddLine(
                            {
                                render_node_editor_position.x + x - style.WindowPadding.x + x_offset,
                                render_node_editor_position.y - style.WindowPadding.y,
                            },
                            {
                                render_node_editor_position.x + x - style.WindowPadding.x + x_offset,
                                render_node_editor_position.y + render_node_editor_size.y + style.WindowPadding.y,
                            },
                            color);
                    }

                    for (float y = 0; y < render_node_editor_size.y; y += GRID_Y) {
                        render_list->AddLine(
                            {
                                render_node_editor_position.x - style.WindowPadding.x,
                                render_node_editor_position.y + y - style.WindowPadding.y + y_offset,
                            },
                            {
                                render_node_editor_position.x + render_node_editor_size.x + style.WindowPadding.x,
                                render_node_editor_position.y + y - style.WindowPadding.y + y_offset,
                            },
                            color);
                    }
                }

                // style.ScaleAllSizes(1);

                std::set<NodeId> nodes_to_delete;

                for (auto& id_node : render_node_graph.eval_node_graph.node_graph.nodes()) {
                    std::visit(
                        [&](auto& node) {
                            EvaluatedNodeGraph::LoadedNode& eval_node = render_node_graph.eval_node_graph.loaded_nodes[node.id];

                            auto node_error = eval_node.error_message;

                            if (node_error.has_value())
                                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.7f, 0.7f, 1.0f));

                            ImGui::SetNextWindowPos(
                                ImVec2{
                                    node.pos.first + render_node_graph.scene_translation.x,
                                    node.pos.second + render_node_graph.scene_translation.y,
                                },
                                ImGuiCond_Always);
                            ImGui::BeginChild(std::to_string(node.id).c_str(), {0, 0}, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
                            {

                                if (ImGui::IsWindowHovered()) {
                                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                                        node.pos.first += io.MouseDelta.x;
                                        node.pos.second += io.MouseDelta.y;
                                    }
                                }

                                node_render(&render_node_graph, node);

                                if (node_error.has_value()) {
                                    ImGui::Separator();
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
                        render_node_graph.scene_translation.x += io.MouseDelta.x;
                        render_node_graph.scene_translation.y += io.MouseDelta.y;
                    }
                }

                ImGui::GetForegroundDrawList()->PopClipRect();
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

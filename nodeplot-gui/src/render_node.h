#pragma once

#include <filesystem>
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <portable-file-dialogs.h>

#include "nodeplot/nodeplot.h"
#include "render_node_graph.h"
#include "style.h"

auto default_input_element = overloaded{
    [](RenderNodeGraph* rng, auto& node, const char* id, std::string& storage) { return ImGui::InputText("##", &storage, ImGuiInputTextFlags_None); },
    [](RenderNodeGraph* rng, auto& node, const char* id, ColumnName& storage) {
        bool res = false;
        bool has_auto_fill = false;

        std::apply(
            [&](auto&&... args) {
                (
                    // TODO: Should probabaly check for something other than table
                    [&]() {
                        if (std::get<1>(args) == "table") {
                            overloaded{
                                [&]<typename T>(InputPin<T> pin) {
                                    if (auto v = rng->eval_node_graph.get_output(pin.node, pin.output_index); v.has_value()) {
                                        std::visit(overloaded{
                                                       [&](Table t) {
                                                           has_auto_fill = true;
                                                           ImGui::SameLine();
                                                           ImGui::SetNextItemWidth(ImGui::GetFrameHeight());
                                                           if (ImGui::BeginCombo("##choose_column", "")) {

                                                               for (auto n : t.column_names) {
                                                                   if (ImGui::Selectable(n.c_str(), false)) {
                                                                       storage.name = n;
                                                                       res = true;
                                                                   }
                                                               }

                                                               ImGui::EndCombo();
                                                           }
                                                       },
                                                       [](auto) {},
                                                   },
                                                   v.value());
                                    }
                                },
                                [](auto) {},
                            }(std::get<0>(args));
                        };
                    }(),
                    ...);
            },
            node.inputs());

        if (has_auto_fill)
            ImGui::SameLine();
        res |= ImGui::InputText("##", &storage.name, ImGuiInputTextFlags_None);

        return res;
    },
    [](RenderNodeGraph* rng, auto& node, const char* id, std::filesystem::path& storage) {
        bool res = false;

        if (ImGui::Button("File Select")) {
            auto selection = pfd::open_file("Choose CSV File", storage.remove_filename(), {"CSV Files", "*.csv", "All Files", "*"}, pfd::opt::none).result();
            if (!selection.empty()) {
                storage = std::filesystem::relative(std::filesystem::path(selection.front()), rng->eval_node_graph.node_graph.file_path().parent_path());
                rng->update_target(node.id);
                res = true;
            }
        }

        ImGui::SameLine();

        res |= ImGui::InputText("##", const_cast<std::string*>(&storage.native()), ImGuiInputTextFlags_None);

        return res;
    },
    [](RenderNodeGraph* rng, auto& node, const char* id, bool& storage) { return ImGui::Checkbox("##", &storage); },
    [](RenderNodeGraph* rng, auto& node, const char* id, double& storage) { return ImGui::InputDouble("##", &storage); },
    [](RenderNodeGraph* rng, auto& node, const char* id, int32_t& storage) { return ImGui::InputInt("##", &storage); },
    [](RenderNodeGraph* rng, auto& node, const char* id, Color& storage) { return ImGui::ColorEdit4("##", (float*)&storage); },
    [](RenderNodeGraph* rng, auto& node, const char* id, Margins& storage) { return ImGui::InputFloat4("##", (float*)&storage); },
    [](RenderNodeGraph* rng, auto& node, const char* id, void*& storage) {},
};

template <typename T>
void node_render(RenderNodeGraph* rng, T& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            std::apply(
                [&](auto&&... args) {
                    (
                        [&]() {
                            auto insert_input = [&](auto& storage) {
                                if constexpr (std::tuple_size_v<std::remove_reference_t<decltype(args)>> == 4) {
                                    auto& map = std::get<3>(args);
                                    input(storage, std::get<1>(args), std::get<2>(args), [&](RenderNodeGraph* rng, T& node, const char* input_id, auto& storage) {
                                        bool res = false;
                                        auto preview_value = [&]() {
                                            if (auto v = map.find(storage); v != map.end())
                                                return v->second;
                                            return std::string("ERR");
                                        }();
                                        ImGui::SetNextItemWidth(ImGui::CalcTextSize(preview_value.c_str()).x + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x);
                                        if (ImGui::BeginCombo("##", preview_value.c_str())) {

                                            for (auto e : map) {
                                                if (ImGui::Selectable(e.second.c_str(), false)) {
                                                    storage = e.first;
                                                    res = true;
                                                }
                                            }

                                            ImGui::EndCombo();
                                        }
                                        return res;
                                    });
                                } else {
                                    input(storage, std::get<1>(args), std::get<2>(args), default_input_element);
                                }
                            };

                            overloaded{[&]<typename V>(std::vector<V>& vec) {
                                           void* empty = nullptr;
                                           input(empty, std::get<1>(args), std::get<2>(args), [&](RenderNodeGraph* rng, T& node, const char* input_id, auto& storage) {
                                               bool res = false;

                                               if (ImGui::Button("+##add_item")) {
                                                   vec.emplace_back();
                                                   res = true;
                                               }

                                               if (ImGui::BeginDragDropTarget()) {
                                                   if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PIN")) {
                                                       IM_ASSERT(payload->DataSize == sizeof(InputPin<void>));
                                                       InputPin<void> payload_n = *(const InputPin<void>*)payload->Data;

                                                       // TODO: Check for infinite loops

                                                       overloaded{
                                                           [&]<typename U>(Input<U> input) { vec.push_back(*(reinterpret_cast<InputPin<U>*>(&payload_n))); },
                                                           [&]<typename U>(InputPin<U> pin) { vec.push_back(*(reinterpret_cast<InputPin<U>*>(&payload_n))); },
                                                       }(V{});

                                                       rng->update_target(node.id);
                                                       res = true;
                                                   }
                                                   ImGui::EndDragDropTarget();
                                               }

                                               return res;
                                           });

                                           for (size_t i = 0; i < vec.size(); i++) {
                                               ImGui::PushID(i);
                                               insert_input(vec[i]);
                                               if (ImGui::Button("-##delete_item")) {
                                                   vec.erase(vec.begin() + i);
                                                   i--;
                                               }
                                               ImGui::PopID();
                                           }
                                       },
                                       [&](auto& v) { insert_input(v); }}(std::get<0>(args));
                        }(),
                        ...);
                },
                node.inputs());
        },
        [&](auto output) { std::apply([&](auto&&... args) { ((output(std::get<0>(args), std::get<1>(args))), ...); }, node.outputs()); });
}

void node_generic_render(RenderNodeGraph* rng, const char* window_title, auto& node, auto get_inputs, auto get_outputs) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImDrawList* fg_draw_list = ImGui::GetForegroundDrawList();

    auto draw_circle = [&](ImVec2 center, float r, ImVec4 color = ImVec4(0.3f, 0.3f, 0.3f, 1.0f)) { draw_list->AddCircle(center, r, ImGui::GetColorU32(color), 0); };

    size_t y_pos = NODE_ELEMENT_HEIGHT / 2.0f;
    ImGui::SetCursorPos({NODE_LEFT_PADDING * rng->scene_scale, y_pos * rng->scene_scale});
    ImGui::Text("%s", window_title);

    size_t imgui_index = 0;

    get_inputs([&](auto& storage, const char* id, const char* input_label, auto gui_element) {
        ImGui::PushID(imgui_index++);

        y_pos += NODE_ELEMENT_HEIGHT;

        ImGui::SetCursorPos({NODE_LEFT_PADDING * rng->scene_scale, y_pos * rng->scene_scale});

        auto drop_target = [&]() {
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PIN")) {
                    IM_ASSERT(payload->DataSize == sizeof(InputPin<void>));
                    InputPin<void> payload_n = *(const InputPin<void>*)payload->Data;

                    // TODO: Check for infinite loops

                    overloaded{
                        [&]<typename T>(Input<T>& input) { input = *(reinterpret_cast<InputPin<T>*>(&payload_n)); },
                        [&]<typename T>(InputPin<T>& pin) { pin = *(reinterpret_cast<InputPin<T>*>(&payload_n)); },
                        [&]<typename T>(T& input) {},
                    }(storage);

                    rng->update_target(node.id);
                }

                ImGui::EndDragDropTarget();
            }
        };

        overloaded{
            [&]<typename T>(Input<T>& input) {
                draw_circle(
                    ImVec2{
                        ImGui::GetCursorScreenPos().x + NODE_IO_CIRCLE_RADIUS * rng->scene_scale,
                        ImGui::GetCursorScreenPos().y + NODE_IO_CIRCLE_RADIUS * rng->scene_scale,
                    },
                    NODE_IO_CIRCLE_RADIUS * rng->scene_scale);
                if (ImGui::InvisibleButton(id, {NODE_IO_CIRCLE_RADIUS * 2.0f * rng->scene_scale, NODE_IO_CIRCLE_RADIUS * 2.0f * rng->scene_scale})) {
                    input = T{};
                    rng->update_target(node.id);
                }
                drop_target();
            },
            [&]<typename T>(InputPin<T>& pin) {
                draw_circle(
                    ImVec2{
                        ImGui::GetCursorScreenPos().x + NODE_IO_CIRCLE_RADIUS * rng->scene_scale,
                        ImGui::GetCursorScreenPos().y + NODE_IO_CIRCLE_RADIUS * rng->scene_scale,
                    },
                    NODE_IO_CIRCLE_RADIUS * rng->scene_scale);
                if (ImGui::InvisibleButton(id, {NODE_IO_CIRCLE_RADIUS * 2.0f * rng->scene_scale, NODE_IO_CIRCLE_RADIUS * 2.0f * rng->scene_scale})) {
                    pin = InputPin<T>{.node = -1, .output_index = ""};
                    rng->update_target(node.id);
                }
                drop_target();
            },
            [&]<typename T>(T& input) {},
        }(storage);

        ImGui::SetCursorPos({(NODE_IO_CIRCLE_RADIUS * 2 + 4.0f) * rng->scene_scale + NODE_IO_CIRCLE_RADIUS * 2.0f * rng->scene_scale, y_pos * rng->scene_scale});
        ImGui::Text("%s", input_label);
        ImGui::SameLine();

        overloaded{
            [&]<typename T>(InputPin<T>&) {},
            [&]<typename T>(Input<T>& i) {
                if (std::holds_alternative<T>(i)) {
                    if (gui_element(rng, node, id, std::get<T>(i))) {
                        rng->update_target(node.id);
                    }
                }
            },
            [&]<typename T>(T& i) {
                if (gui_element(rng, node, id, i)) {
                    rng->update_target(node.id);
                }
            },
        }(storage);

        ImGui::PopID();
    });

    y_pos += NODE_ELEMENT_HEIGHT;
    ImGui::SetCursorPos({NODE_LEFT_PADDING * rng->scene_scale, (y_pos + 11) * rng->scene_scale});
    ImGui::Separator();

    get_outputs([&](const char* id, const char* label) {
        y_pos += NODE_ELEMENT_HEIGHT;
        ImGui::SetCursorPos({NODE_LEFT_PADDING * rng->scene_scale, y_pos * rng->scene_scale});

        ImGui::Text("%s", label);

        ImGui::SetCursorPos({ImGui::GetWindowWidth() - (NODE_LEFT_PADDING + NODE_IO_CIRCLE_RADIUS) * rng->scene_scale, y_pos * rng->scene_scale});

        ImVec2 circle_center = ImVec2{
            ImGui::GetCursorScreenPos().x + NODE_IO_CIRCLE_RADIUS * rng->scene_scale,
            ImGui::GetCursorScreenPos().y + NODE_IO_CIRCLE_RADIUS * rng->scene_scale,
        };

        draw_circle(circle_center, NODE_IO_CIRCLE_RADIUS * rng->scene_scale);

        ImGui::InvisibleButton(id, {NODE_IO_CIRCLE_RADIUS * 2.0f, NODE_IO_CIRCLE_RADIUS * 2.0f});
        if (ImGui::BeginDragDropSource()) {
            InputPin<void> payload;
            payload.node = node.id;
            payload.output_index = id;
            ImGui::SetDragDropPayload("PIN", &payload, sizeof(payload));
            ImGui::EndDragDropSource();

            rng->draw_node_path(circle_center, ImGui::GetIO().MousePos);
        }
    });
}
#pragma once

#include <concepts>
#include <filesystem>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <portable-file-dialogs.h>
#include <tuple>
#include <type_traits>
#include <vector>

#include "nodeplot/nodeplot.h"
#include "render_node_graph.h"

auto default_input_element = overloaded{
    [](RenderNodeGraph* rng, auto& node, const char* id, std::string& storage) {
        ImGui::SetNextItemWidth(200 * rng->scene_scale);
        return ImGui::InputText("##", &storage, ImGuiInputTextFlags_None);
    },
    [](RenderNodeGraph* rng, auto& node, const char* id, ColumnName& storage) {
        ImGui::SetNextItemWidth(200 * rng->scene_scale);
        bool res = false;
        res |= ImGui::InputText("##", &storage.name, ImGuiInputTextFlags_None);

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

        return res;
    },
    [](RenderNodeGraph* rng, auto& node, const char* id, std::filesystem::path& storage) {
        ImGui::SetNextItemWidth(200 * rng->scene_scale);
        bool text_input = ImGui::InputText("##", const_cast<std::string*>(&storage.native()), ImGuiInputTextFlags_None);
        if (ImGui::Button("File Select")) {
            auto selection = pfd::open_file("Choose CSV File", storage.remove_filename(), {"CSV Files", "*.csv", "All Files", "*"}, pfd::opt::none).result();
            if (!selection.empty()) {
                storage = std::filesystem::relative(std::filesystem::path(selection.front()), rng->eval_node_graph.node_graph.file_path().parent_path());
                rng->update_target(node.id);
                return true;
            }
        }
        return text_input;
    },
    [](RenderNodeGraph* rng, auto& node, const char* id, bool& storage) { return ImGui::Checkbox("##", &storage); },
    [](RenderNodeGraph* rng, auto& node, const char* id, double& storage) {
        ImGui::SetNextItemWidth(200 * rng->scene_scale);
        return ImGui::InputDouble("##", &storage);
    },
    [](RenderNodeGraph* rng, auto& node, const char* id, int32_t& storage) {
        ImGui::SetNextItemWidth(200 * rng->scene_scale);
        return ImGui::InputInt("##", &storage);
    },
    [](RenderNodeGraph* rng, auto& node, const char* id, Color& storage) {
        ImGui::SetNextItemWidth(200 * rng->scene_scale);
        return ImGui::ColorEdit4("##", (float*)&storage);
    },
    [](RenderNodeGraph* rng, auto& node, const char* id, Margins& storage) {
        ImGui::SetNextItemWidth(200 * rng->scene_scale);
        return ImGui::InputFloat4("##", (float*)&storage);
    },
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
                                           ImGui::TableNextRow();

                                           ImGui::TableNextColumn();
                                           ImGui::TableNextColumn();
                                           ImGui::TableNextColumn();
                                           ImGui::TableNextColumn();
                                           ImGui::TableNextColumn();

                                           if (ImGui::Button("+##add_item")) {
                                               vec.emplace_back();
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
                                               }
                                               ImGui::EndDragDropTarget();
                                           }

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
        [&](auto output) { std::apply([&](auto&&... args) { ((output(std::get<0>(args), std::get<1>(args), std::get<2>(args))), ...); }, node.outputs()); });
}

void node_generic_render(RenderNodeGraph* rng, const char* window_title, auto& node, auto get_inputs, auto get_outputs) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImDrawList* fg_draw_list = ImGui::GetForegroundDrawList();

    auto draw_circle = [&](ImVec2 center, float r, ImVec4 color = ImVec4(0.3f, 0.3f, 0.3f, 1.0f)) { draw_list->AddCircle(center, r, ImGui::GetColorU32(color), 0); };

    auto render_path = [](ImVec2 start, ImVec2 end) {
        constexpr ImVec4 color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        const ImU32 color_32 = ImGui::GetColorU32(color);

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        // TODO: Clip Rec
        //  draw_list->PushClipRect(const ImVec2 &clip_rect_min, const ImVec2 &clip_rect_max)

        // fg_draw_list->AddLine(start, end, color_32);

        if (end.x < start.x) {
            ImVec2 half = {(start.x + end.x) / 2, (start.y + end.y) / 2};

            // draw_list->AddLine(start, half, color_32);
            // draw_list->AddLine(half, end, color_32);

            float start_end_offset = 50;
            float half_offset = std::clamp(std::abs(start.x - end.x), 50.0f, 200.0f);

            draw_list->AddBezierCubic(start, {start.x + start_end_offset, start.y}, {half.x + half_offset, half.y}, half, color_32, 3);
            draw_list->AddBezierCubic(half, {half.x - half_offset, half.y}, {end.x - start_end_offset, end.y}, end, color_32, 3);
        } else {
            float diff = std::abs(end.x - start.x);

            float control_offset = std::min(200.0f, diff);

            ImVec2 s_control = ImVec2(start.x + control_offset, start.y);
            ImVec2 e_control = ImVec2(end.x - control_offset, end.y);

            draw_list->AddBezierCubic(start, s_control, e_control, end, color_32, 3);
        }
    };

    ImGui::Text("%s", window_title);

    // Inputs
    if (ImGui::BeginTable("input_layout", 5)) {

        size_t imgui_index = 0;

        get_inputs([&](auto& storage, const char* id, const char* input_label, auto gui_element) {
            ImGui::PushID(imgui_index++);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TableNextColumn();

            float dd_target_size = 10.0f * rng->scene_scale;

            auto render_pin = [&]<typename T>(InputPin<T> pin, ImVec4 color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f)) {
                auto other_node = rng->render_nodes.find(pin.node);
                if (other_node == rng->render_nodes.end())
                    return;

                auto other_pin_pos = other_node->second.render_pin_positions.find(pin.output_index);
                if (other_pin_pos == other_node->second.render_pin_positions.end())
                    return;

                ImDrawList* draw_list = ImGui::GetForegroundDrawList();

                ImVec2 pos = ImGui::GetCursorScreenPos();
                render_path(rng->world_to_screen(other_pin_pos->second), ImVec2(pos.x + dd_target_size / 2.0f, pos.y + dd_target_size / 2.0f));
            };

            overloaded{
                [&]<typename T>(Input<T>& input) {
                    if (std::holds_alternative<InputPin<T>>(input))
                        render_pin(std::get<InputPin<T>>(input));
                },
                [&]<typename T>(InputPin<T>& pin) { render_pin(pin); },
            }(storage);

            ImVec2 pin_source = ImGui::GetCursorScreenPos();
            pin_source.x += dd_target_size / 2.0f;
            pin_source.y += dd_target_size / 2.0f;

            draw_circle(pin_source, dd_target_size / 2.0f);
            if (ImGui::InvisibleButton(id, {dd_target_size, dd_target_size})) {
                overloaded{
                    [&]<typename T>(Input<T>& input) { input = T{}; },
                    [&]<typename T>(InputPin<T>& pin) { pin = InputPin<T>{.node = -1, .output_index = -1}; },
                }(storage);
                rng->update_target(node.id);
            }
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PIN")) {
                    IM_ASSERT(payload->DataSize == sizeof(InputPin<void>));
                    InputPin<void> payload_n = *(const InputPin<void>*)payload->Data;

                    // TODO: Check for infinite loops

                    overloaded{
                        [&]<typename T>(Input<T>& input) { input = *(reinterpret_cast<InputPin<T>*>(&payload_n)); },
                        [&]<typename T>(InputPin<T>& pin) { pin = *(reinterpret_cast<InputPin<T>*>(&payload_n)); },
                    }(storage);

                    rng->update_target(node.id);
                }

                ImGui::EndDragDropTarget();
            }

            ImGui::TableNextColumn();
            ImGui::Text("%s", input_label);
            ImGui::TableNextColumn();

            overloaded{
                [&]<typename T>(InputPin<T>&) {},
                [&]<typename T>(Input<T>& i) {
                    if (std::holds_alternative<T>(i)) {
                        if (gui_element(rng, node, id, std::get<T>(i))) {
                            rng->update_target(node.id);
                        }
                    }
                },
            }(storage);

            ImGui::TableNextColumn();
            ImGui::PopID();
        });

        ImGui::EndTable();
    }

    bool first_output = true;

    get_outputs([&](OutputIndex index, const char* id, const char* label) {
        if (first_output) {
            first_output = false;
            ImGui::Separator();
        }

        ImGui::Text("%s", label);
        ImGui::SameLine(ImGui::GetColumnWidth() - 10 * rng->scene_scale);

        float dd_target_size = 10.0f * rng->scene_scale;

        ImVec2 pin_source = ImGui::GetCursorScreenPos();
        pin_source.x += dd_target_size / 2.0f;
        pin_source.y += dd_target_size / 2.0f;

        rng->render_nodes[node.id].render_pin_positions[index] = rng->screen_to_world(pin_source);

        draw_circle(pin_source, dd_target_size / 2.0f);
        ImGui::InvisibleButton(id, {dd_target_size, dd_target_size});
        if (ImGui::BeginDragDropSource()) {
            InputPin<void> payload;
            payload.node = node.id;
            payload.output_index = index;
            ImGui::SetDragDropPayload("PIN", &payload, sizeof(payload));

            render_path(pin_source, ImGui::GetIO().MousePos);

            ImGui::EndDragDropSource();
        }
    });
}
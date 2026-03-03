#pragma once

#include <filesystem>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <portable-file-dialogs.h>
#include <type_traits>

#include "nodeplot/nodeplot.h"
#include "render_node_graph.h"

template <typename T>
void node_render(RenderNodeGraph* rng, T& node) {
    node_generic_render(
        rng,
        node.name().c_str(),
        node,
        [&](auto input) {
            std::apply(
                [&](auto&&... args) { ((input((std::string("##i_") + std::get<1>(args)).c_str(), (std::string("##ii_") + std::get<1>(args)).c_str(), std::get<2>(args), std::get<0>(args))), ...); },
                node.inputs());
        },
        [&](auto output) { std::apply([&](auto&&... args) { ((output((std::string("##o_") + std::get<1>(args)).c_str(), std::get<0>(args), std::get<2>(args))), ...); }, node.outputs()); });
}

void node_generic_render(RenderNodeGraph* rng, const char* window_title, auto node, auto get_inputs, auto get_outputs) {
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

        auto render_input = [&](const char* id, const char* input_id, const char* input_label, auto& input_refrence, auto inline_input_element) {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TableNextColumn();

            auto render_pin = [&]<typename T>(InputPin<T> pin, ImVec4 color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f)) {
                auto other_node = rng->render_nodes.find(pin.node);
                if (other_node == rng->render_nodes.end())
                    return;

                auto other_pin_pos = other_node->second.render_pin_positions.find(pin.output_index);
                if (other_pin_pos == other_node->second.render_pin_positions.end())
                    return;

                ImDrawList* draw_list = ImGui::GetForegroundDrawList();

                ImVec2 pos = ImGui::GetCursorScreenPos();
                render_path(
                    {
                        other_pin_pos->second.x + rng->scene_translation.x,
                        other_pin_pos->second.y + rng->scene_translation.y,
                    },
                    ImVec2(pos.x + 5, pos.y + 5));
            };

            overloaded{
                [&]<typename T>(Input<T>& input) {
                    if (std::holds_alternative<InputPin<T>>(input))
                        render_pin(std::get<InputPin<T>>(input));
                },
                [&]<typename T>(InputPin<T>& pin) { render_pin(pin); },
            }(input_refrence);

            ImVec2 pin_source = ImGui::GetCursorScreenPos();
            pin_source.x += 5;
            pin_source.y += 5;

            draw_circle(pin_source, 5);
            if (ImGui::InvisibleButton(id, {10, 10})) {
                overloaded{
                    [&]<typename T>(Input<T>& input) { input = T{}; },
                    [&]<typename T>(InputPin<T>& pin) { pin = InputPin<T>{.node = -1, .output_index = -1}; },
                }(input_refrence);
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
                    }(input_refrence);

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
                        if (inline_input_element(input_id, std::get<T>(i))) {
                            rng->update_target(node.id);
                        }
                    }
                },
            }(input_refrence);

            ImGui::TableNextColumn();
        };

        auto default_input_element = overloaded{
            [&](const char* input_id, std::string& input) {
                ImGui::SetNextItemWidth(200);
                return ImGui::InputText(input_id, &input, ImGuiInputTextFlags_None);
            },
            [&](const char* input_id, ColumnName& input) {
                ImGui::SetNextItemWidth(200);
                bool res = false;
                res |= ImGui::InputText(input_id, &input.name, ImGuiInputTextFlags_None);

                std::apply(
                    [&](auto&&... args) {
                        (
                            // TODO: Should probabaly check for something other than table
                            [&]() {
                                if (std::get<1>(args) == "table") {
                                    overloaded{
                                        [&]<typename T>(InputPin<T> pin) {
                                            if (auto lng = rng->eval_node_graph.loaded_nodes.find(pin.node); lng != rng->eval_node_graph.loaded_nodes.end()) {
                                                if (auto v = lng->second.cache.find(pin.output_index); v != lng->second.cache.end()) {
                                                    std::visit(overloaded{
                                                                   [&](Table t) {
                                                                       ImGui::SameLine();
                                                                       ImGui::SetNextItemWidth(ImGui::GetFrameHeight());
                                                                       if (ImGui::BeginCombo("##choose_column", "")) {

                                                                           for (auto n : t.column_names) {
                                                                               if (ImGui::Selectable(n.c_str(), false)) {
                                                                                   input.name = n;
                                                                                   res = true;
                                                                               }
                                                                           }

                                                                           ImGui::EndCombo();
                                                                       }
                                                                   },
                                                                   [](auto) {},
                                                               },
                                                               v->second);
                                                }
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
            [&](const char* input_id, std::filesystem::path& input) {
                ImGui::SetNextItemWidth(200);
                bool text_input = ImGui::InputText(input_id, const_cast<std::string*>(&input.native()), ImGuiInputTextFlags_None);
                if (ImGui::Button("File Select")) {
                    auto selection = pfd::open_file("Choose CSV File", input.remove_filename(), {"CSV Files", "*.csv", "All Files", "*"}, pfd::opt::none).result();
                    if (!selection.empty()) {
                        input = std::filesystem::relative(std::filesystem::path(selection.front()), rng->eval_node_graph.node_graph.file_path().parent_path());
                        rng->update_target(node.id);
                        return true;
                    }
                }
                return text_input;
            },
            [&](const char* input_id, bool& input) { return ImGui::Checkbox(input_id, &input); },
            [&](const char* input_id, double& input) {
                ImGui::SetNextItemWidth(200);
                return ImGui::InputDouble(input_id, &input);
            },
            [&](const char* input_id, int32_t& input) {
                ImGui::SetNextItemWidth(200);
                return ImGui::InputInt(input_id, &input);
            },
            [&](const char* input_id, Color& input) {
                ImGui::SetNextItemWidth(200);
                return ImGui::ColorEdit4(input_id, (float*)&input);
            },
            [&](const char* input_id, Margins& input) {
                ImGui::SetNextItemWidth(200);
                return ImGui::InputFloat4(input_id, (float*)&input);
            },
        };

        auto normal_input = [&](const char* id, const char* input_id, const char* input_label, auto& input_refrence) {
            overloaded{
                [&]<typename T>(std::vector<T>& list) {
                    size_t index = 0;
                    for (auto it = list.begin(); it != list.end();) {
                        ImGui::PushID(index);
                        render_input(id, input_id, input_label, *it, default_input_element);
                        if (ImGui::Button("-")) {
                            it = list.erase(it);
                            rng->update_target(node.id);
                        } else {
                            ++it;
                        }
                        ImGui::PopID();
                        index++;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();

                    if (ImGui::Button("+")) {
                        list.emplace_back(T{});
                        rng->update_target(node.id);
                    }
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PIN")) {
                            IM_ASSERT(payload->DataSize == sizeof(InputPin<void>));
                            InputPin<void> payload_n = *(const InputPin<void>*)payload->Data;

                            // TODO: Check for infinite loops

                            list.emplace_back(T{});

                            overloaded{
                                [&]<typename T2>(Input<T2>& input) { input = *(reinterpret_cast<InputPin<T2>*>(&payload_n)); },
                                [&]<typename T2>(InputPin<T2>& pin) { pin = *(reinterpret_cast<InputPin<T2>*>(&payload_n)); },
                            }(list.back());

                            rng->update_target(node.id);
                        }

                        ImGui::EndDragDropTarget();
                    }
                },
                [&]<typename T>(Input<T>& input) { render_input(id, input_id, input_label, input, default_input_element); },
                [&]<typename T>(InputPin<T>& pin) { render_input(id, input_id, input_label, pin, default_input_element); },
            }(input_refrence);
        };

        if constexpr (std::is_invocable_v<decltype(get_inputs), decltype(normal_input), decltype(render_input)>) {
            get_inputs(normal_input, render_input);
        } else {
            get_inputs(normal_input);
        }

        ImGui::EndTable();
    }

    bool first_output = true;

    get_outputs([&](const char* id, OutputId output_id, const char* output_label) {
        if (first_output) {
            first_output = false;
            ImGui::Separator();
        }

        ImGui::PushItemWidth(-1.0f);
        ImGui::Text("%s", output_label);
        ImGui::SameLine(ImGui::GetColumnWidth() - 10);

        ImVec2 pin_source = ImGui::GetCursorScreenPos();
        pin_source.x += 5;
        pin_source.y += 5;

        {
            rng->render_nodes[node.id].render_pin_positions[output_id] = {
                pin_source.x - rng->scene_translation.x,
                pin_source.y - rng->scene_translation.y,
            };
        }

        draw_circle(pin_source, 5);
        ImGui::InvisibleButton(id, {10, 10});
        // ImGui::BeginDragDropTarget();
        if (ImGui::BeginDragDropSource()) {
            InputPin<void> payload;
            payload.node = node.id;
            payload.output_index = output_id;
            ImGui::SetDragDropPayload("PIN", &payload, sizeof(payload));

            render_path(pin_source, ImGui::GetIO().MousePos);

            ImGui::EndDragDropSource();
        }

        ImGui::PopItemWidth();
    });
}
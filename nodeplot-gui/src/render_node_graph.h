#pragma once

#include <nodeplot/nodeplot.h>

#include "style.h"
#include "svg_renderer.h"

struct RenderNodeGraph {
    float scene_scale = 1;
    ImVec2 scene_translation = {};

    EvaluatedNodeGraph eval_node_graph;

    std::shared_ptr<SVGRenderer> svg_renderer;

    struct NodeRenderInfo {};
    std::map<int64_t, NodeRenderInfo> render_nodes;

    void update_target(NodeId updated_node = -1);

    ImVec2 world_to_screen(ImVec2 p) { return ImVec2((p.x - scene_translation.x) * scene_scale, (p.y - scene_translation.y) * scene_scale); }
    ImVec2 screen_to_world(ImVec2 p) { return ImVec2(p.x / scene_scale + scene_translation.x, p.y / scene_scale + scene_translation.y); }

    void draw_node_path(ImVec2 start, ImVec2 end) {
        // ImGui::GetForegroundDrawList()->AddLine(start, end, ImGui::GetColorU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)));

        constexpr ImVec4 color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        const ImU32 color_32 = ImGui::GetColorU32(color);

        ImDrawList* fg_draw_list = ImGui::GetForegroundDrawList();
        ImDrawList* bg_draw_list = ImGui::GetBackgroundDrawList();

        float PATH_X_ESCAPE = 20.0f * scene_scale;

        fg_draw_list->AddLine(start,
                              {
                                  start.x + PATH_X_ESCAPE,
                                  start.y,
                              },
                              color_32,
                              3);

        fg_draw_list->AddLine(end,
                              {
                                  end.x - PATH_X_ESCAPE,
                                  end.y,
                              },
                              color_32,
                              3);

        start = {
            start.x + PATH_X_ESCAPE,
            start.y,
        };

        end = {
            end.x - PATH_X_ESCAPE,
            end.y,
        };

        if (end.x < start.x) {
            ImVec2 half = {(start.x + end.x) / 2, (start.y + end.y) / 2};

            float start_end_offset = 50;
            float half_offset = std::clamp(std::abs(start.x - end.x), 50.0f, 200.0f);

            bg_draw_list->AddBezierCubic(start, {start.x + start_end_offset, start.y}, {half.x + half_offset, half.y}, half, color_32, 3);
            bg_draw_list->AddBezierCubic(half, {half.x - half_offset, half.y}, {end.x - start_end_offset, end.y}, end, color_32, 3);
        } else {
            float diff = std::abs(end.x - start.x);

            float control_offset = std::min(200.0f, diff);

            ImVec2 s_control = ImVec2(start.x + control_offset, start.y);
            ImVec2 e_control = ImVec2(end.x - control_offset, end.y);

            bg_draw_list->AddBezierCubic(start, s_control, e_control, end, color_32, 3);
        }
    }

    constexpr size_t get_header_height(auto node) {
        size_t height = 0;
        height += NODE_ELEMENT_HEIGHT / 2;
        height += NODE_ELEMENT_HEIGHT;
        return height;
    }

    constexpr size_t get_input_height(auto node) {
        size_t height = 0;
        for_each_tuple(
            [&](auto tup) {
                overloaded{
                    [&]<typename T>(std::vector<T>& v) {
                        for (auto& a : v) {
                            height += NODE_ELEMENT_HEIGHT;
                        }
                        height += NODE_ELEMENT_HEIGHT;
                    },
                    [&](auto&) { height += NODE_ELEMENT_HEIGHT; },
                }(std::get<NODE_INPUT_INDEX_STORAGE>(tup));
            },
            node.inputs());
        return height;
    }

    constexpr size_t get_output_height(auto node) {
        size_t height = 0;
        for_each_tuple([&](auto) { height += NODE_ELEMENT_HEIGHT; }, node.outputs());
        return height;
    }
};

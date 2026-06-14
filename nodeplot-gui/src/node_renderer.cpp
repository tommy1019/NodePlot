#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <imgui.h>
#include <nfd.h>

#include "node_renderer.h"
#include "nodeplot/node.h"
#include "nodeplot/node_graph.h"
#include "nodeplot/nodeplot.h"
#include "nodeplot/types.h"

std::map<NodePlot::NodeTypeId, NodeRenderer::RenderFunction> NodeRenderer::render_override_map;

NodeRenderer::RenderFunction NodeRenderer::default_renderer = [](Renderer& rnd, RenderContext& ctx) -> ErrorOr<bool> {
    bool updated = false;

    float rs = ctx.node_renderer.scene_scale;

    float INPUT_HEIGHT = 18 * rs;
    float PADDING = 6 * rs;

    float PIN_SIZE = 6 * rs;

    float INPUT_PIN_WIDTH = 15 * rs;
    float INPUT_TEXT_WIDTH = 150 * rs;
    float INPUT_ELEMENT_WIDTH = 200 * rs;

    float OUTPUT_PIN_X = INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH + INPUT_ELEMENT_WIDTH + PADDING;

    float y_pos = -INPUT_HEIGHT + PADDING;

    rnd.text(ctx, {PADDING, y_pos += INPUT_HEIGHT}, ctx.node.display_name);
    rnd.separator(ctx, {PADDING / 2, (y_pos += INPUT_HEIGHT * 0.75f) + (INPUT_HEIGHT * 0.75f) / 2.0f});

    auto inputs = TRY(ctx.node.inputs(ctx.npf, ctx.eng, ctx.node_id));
    for (auto [id, input] : inputs) {
        ImGui::PushID(id.c_str());

        float cur_y = y_pos += INPUT_HEIGHT + 6 * rs;

        if (rnd.input_pin(ctx, {PADDING, cur_y + 2 * rs}, PIN_SIZE, input.id)) {
            updated = true;
        }

        rnd.text(ctx, {PADDING + 15 * rs, cur_y}, input.display_name);

        if (input.valid_data_types.size() > 0) {
            auto input_type = input.valid_data_types.front();

            // For these types we must create the input storage if it doesn't exist to use for the imgui gui elements. Otherwise we do not create storage so the error messages are better

            switch (input_type) {
            case NodePlot::DataType::STRING:
            case NodePlot::DataType::BOOLEAN:
            case NodePlot::DataType::NUMBER:
            case NodePlot::DataType::INTEGER:
            case NodePlot::DataType::COLOR:
            case NodePlot::DataType::MARGINES:
            case NodePlot::DataType::POSITION: {

                std::variant<NodePlot::Data, NodePlot::NodeGraph::InputPin>& input_storage = ctx.node_storage.input_storage[id];
                bool is_pin_set = std::holds_alternative<NodePlot::NodeGraph::InputPin>(input_storage);

                switch (input_type) {
                case NodePlot::DataType::STRING: {
                    float ATTR_OFFSET = 0;

                    for (auto& att : input.attributes) {
                        if (std::holds_alternative<NodePlot::InputAttribute::FilePath>(att)) {
                            ATTR_OFFSET += INPUT_HEIGHT;
                            if (rnd.button(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_HEIGHT, INPUT_HEIGHT}, "...")) {
                                if (!is_pin_set) {
                                    nfdu8char_t* out_path;
                                    nfdopendialogu8args_t args = {0};
                                    nfdu8filteritem_t filters[2] = {{"CSV Files", "csv"}, {"All Files", "*"}};
                                    args.filterList = filters;
                                    args.filterCount = 2;

                                    nfdresult_t result = NFD_OpenDialogU8_With(&out_path, &args);
                                    if (result == NFD_OKAY) {
                                        auto& data = std::get<NodePlot::Data>(input_storage);
                                        data = std::filesystem::relative(std::filesystem::path(std::string(out_path)), ctx.npf->path.parent_path()).string();
                                        updated = true;
                                        NFD_FreePathU8(out_path);
                                    }
                                }
                            }
                        }

                        if (std::holds_alternative<NodePlot::InputAttribute::AutoFillsFromTable>(att)) {
                            auto table = ctx.eng->get_input_value<NodePlot::Table>(ctx.npf, ctx.node_id, std::get<NodePlot::InputAttribute::AutoFillsFromTable>(att).id);
                            if (table.has_value()) {
                                ATTR_OFFSET += INPUT_HEIGHT;

                                std::vector<std::pair<std::string, std::string>> options;
                                options.reserve(table.value()->column_names.size());
                                for (auto& h : table.value()->column_names)
                                    options.emplace_back(h, h);

                                auto res = rnd.dropdown(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_HEIGHT, INPUT_HEIGHT}, "\\/", options);
                                if (res.has_value()) {
                                    if (!is_pin_set) {
                                        auto& data = std::get<NodePlot::Data>(input_storage);
                                        data = res.value();
                                        updated = true;
                                    }
                                }
                            }
                        }

                        if (std::holds_alternative<NodePlot::InputAttribute::FunctionName>(att)) {
                            ATTR_OFFSET += INPUT_HEIGHT;
                            ATTR_OFFSET += INPUT_HEIGHT;
                            ATTR_OFFSET += INPUT_HEIGHT;

                            std::vector<std::pair<std::string, std::string>> options;
                            for (auto [key, graph] : ctx.npf->graphs) {
                                if (key == "main")
                                    continue;
                                options.emplace_back(key, key);
                            }

                            if (rnd.button(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_HEIGHT, INPUT_HEIGHT}, "+")) {
                                if (!is_pin_set) {
                                    auto& data = std::get<NodePlot::Data>(input_storage);
                                    if (std::holds_alternative<std::string>(data)) {
                                        auto fn_name = std::get<std::string>(data);
                                        if (!ctx.npf->graphs.contains(fn_name)) {
                                            NodePlot::NodeGraph new_graph;

                                            new_graph.function_input_id = TRY(new_graph.create_node(ctx.npf, ctx.eng->graph_id, "function_input", 100, 100));
                                            new_graph.function_output_id = TRY(new_graph.create_node(ctx.npf, ctx.eng->graph_id, "function_output", 600, 100));

                                            ctx.npf->graphs.insert({fn_name, new_graph});
                                            updated = true;
                                        }
                                    }
                                }
                            }

                            auto res = rnd.dropdown(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH + INPUT_HEIGHT, cur_y}, {INPUT_HEIGHT, INPUT_HEIGHT}, "##existing_functions", options);
                            if (res.has_value()) {
                                input_storage = NodePlot::Data{res.value()};
                                updated = true;
                            }

                            if (rnd.button(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH + INPUT_HEIGHT * 2, cur_y}, {INPUT_HEIGHT, INPUT_HEIGHT}, "->")) {
                                if (!is_pin_set) {
                                    auto& data = std::get<NodePlot::Data>(input_storage);
                                    if (std::holds_alternative<std::string>(data)) {
                                        auto fn_name = std::get<std::string>(data);
                                        if (ctx.npf->graphs.contains(fn_name)) {
                                            ctx.node_renderer.set_current_graph_id(fn_name);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (is_pin_set) {
                        static std::string disabled_str = "";
                        rnd.string_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH + ATTR_OFFSET, cur_y}, {INPUT_ELEMENT_WIDTH - ATTR_OFFSET, INPUT_HEIGHT}, disabled_str, false);
                    } else {
                        auto& data = std::get<NodePlot::Data>(input_storage);
                        if (!std::holds_alternative<std::string>(data))
                            data = std::string();
                        if (rnd.string_input(
                                ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH + ATTR_OFFSET, cur_y}, {INPUT_ELEMENT_WIDTH - ATTR_OFFSET, INPUT_HEIGHT}, std::get<std::string>(data), true)) {
                            updated = true;
                        }
                    }
                } break;
                case NodePlot::DataType::BOOLEAN:
                    if (is_pin_set) {
                        static bool disabled_bool = false;
                        rnd.boolean_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, disabled_bool, false);
                    } else {
                        auto& data = std::get<NodePlot::Data>(input_storage);
                        if (!std::holds_alternative<bool>(data)) {
                            data = false;
                            updated = true;
                        }
                        if (rnd.boolean_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, std::get<bool>(data), true)) {
                            updated = true;
                        }
                    }
                    break;
                case NodePlot::DataType::NUMBER:
                    if (is_pin_set) {
                        static double disabled_double = 0.0;
                        rnd.number_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_ELEMENT_WIDTH, INPUT_HEIGHT}, disabled_double, false);
                    } else {
                        auto& data = std::get<NodePlot::Data>(input_storage);
                        if (!std::holds_alternative<double>(data)) {
                            data = 0.0;
                            updated = true;
                        }
                        if (rnd.number_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_ELEMENT_WIDTH, INPUT_HEIGHT}, std::get<double>(data), true)) {
                            updated = true;
                        }
                    }
                    break;
                case NodePlot::DataType::INTEGER:
                    if (is_pin_set) {
                        static int64_t disabled_int = 0.0;
                        rnd.integer_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_ELEMENT_WIDTH, INPUT_HEIGHT}, disabled_int, false);
                    } else {
                        auto& data = std::get<NodePlot::Data>(input_storage);
                        if (!std::holds_alternative<int64_t>(data)) {
                            data = 0;
                            updated = true;
                        }
                        if (rnd.integer_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_ELEMENT_WIDTH, INPUT_HEIGHT}, std::get<int64_t>(data), true)) {
                            updated = true;
                        }
                    }
                    break;
                case NodePlot::DataType::COLOR:
                    if (is_pin_set) {
                        static NodePlot::Color disabled_color = {};
                        rnd.color_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_ELEMENT_WIDTH, INPUT_HEIGHT}, disabled_color, false);
                    } else {
                        auto& data = std::get<NodePlot::Data>(input_storage);
                        if (!std::holds_alternative<NodePlot::Color>(data)) {
                            data = NodePlot::Color{};
                            updated = true;
                        }
                        if (rnd.color_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_ELEMENT_WIDTH, INPUT_HEIGHT}, std::get<NodePlot::Color>(data), true)) {
                            updated = true;
                        }
                    }
                    break;
                case NodePlot::DataType::MARGINES:
                    if (is_pin_set) {
                        static NodePlot::Margins disabled_margin = {};
                        rnd.margin_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_ELEMENT_WIDTH, INPUT_HEIGHT}, disabled_margin, false);
                    } else {
                        auto& data = std::get<NodePlot::Data>(input_storage);
                        if (!std::holds_alternative<NodePlot::Margins>(data)) {
                            data = NodePlot::Margins{};
                            updated = true;
                        }
                        if (rnd.margin_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_ELEMENT_WIDTH, INPUT_HEIGHT}, std::get<NodePlot::Margins>(data), true)) {
                            updated = true;
                        }
                    }
                    break;
                case NodePlot::DataType::POSITION:
                    if (is_pin_set) {
                        static NodePlot::Pos disabled_pos = {};
                        rnd.position_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_ELEMENT_WIDTH, INPUT_HEIGHT}, disabled_pos, false);
                    } else {
                        auto& data = std::get<NodePlot::Data>(input_storage);
                        if (!std::holds_alternative<NodePlot::Pos>(data)) {
                            data = NodePlot::Pos{};
                            updated = true;
                        }
                        if (rnd.position_input(ctx, {PADDING + INPUT_PIN_WIDTH + INPUT_TEXT_WIDTH, cur_y}, {INPUT_ELEMENT_WIDTH, INPUT_HEIGHT}, std::get<NodePlot::Pos>(data), true)) {
                            updated = true;
                        }
                    }
                    break;
                default:
                    break;
                }
            } break;
            default:
            }
        }

        ImGui::PopID();
    }

    rnd.separator(ctx, {PADDING / 2, (y_pos += INPUT_HEIGHT * 0.75f) + (INPUT_HEIGHT * 0.75f) / 2.0f});

    auto outputs = TRY(ctx.node.outputs(ctx.npf, ctx.eng, ctx.node_id));
    for (auto [id, output] : outputs) {
        ImGui::PushID(id.c_str());

        float cur_y = y_pos += INPUT_HEIGHT;

        rnd.text(ctx, {PADDING, cur_y}, output.display_name);
        rnd.output_pin(ctx, {OUTPUT_PIN_X, cur_y + 2 * rs}, PIN_SIZE, output.id);

        ImGui::PopID();
    }

    return updated;
};

void NodeRenderer::draw_node_path(ImVec2 start, ImVec2 end) {
    constexpr ImVec4 color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    const ImU32 color_32 = ImGui::GetColorU32(color);

    ImDrawList* fg_draw_list = ImGui::GetForegroundDrawList();
    ImDrawList* bg_draw_list = ImGui::GetBackgroundDrawList();

    float PATH_X_ESCAPE = 20.0f * scene_scale;

    float OVERLAP_MUL = 0.02f;

    fg_draw_list->AddLine(start,
                          {
                              start.x + PATH_X_ESCAPE * (1.0f + OVERLAP_MUL),
                              start.y,
                          },
                          color_32,
                          3);

    fg_draw_list->AddLine(end,
                          {
                              end.x - PATH_X_ESCAPE * (1.0f + OVERLAP_MUL),
                              end.y,
                          },
                          color_32,
                          3);

    start = {
        start.x + PATH_X_ESCAPE * (1.0f - OVERLAP_MUL),
        start.y,
    };

    end = {
        end.x - PATH_X_ESCAPE * (1.0f - OVERLAP_MUL),
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

ErrorOr<bool> NodeRenderer::render_node(NodePlot::NodeId node_id, NodePlot::NodeGraph::NodeStorage& storage) {
    Renderer imgui_renderer{
        .text =
            [](RenderContext&, ImVec2 pos, std::string str) {
                ImGui::SetCursorPos(pos);
                ImGui::Text("%s", str.c_str());
            },
        .separator =
            [](RenderContext&, ImVec2 pos) {
                ImGui::SetCursorPos(pos);
                ImGui::Separator();
            },

        .button =
            [](RenderContext&, ImVec2 pos, ImVec2 size, std::string label) {
                ImGui::SetCursorPos(pos);
                return ImGui::Button(label.c_str(), size);
            },

        .dropdown = [](RenderContext&, ImVec2 pos, ImVec2 size, std::string label, std::vector<std::pair<std::string, std::string>> options) -> std::optional<std::string> {
            ImGui::SetCursorPos(pos);
            ImGui::SetNextItemWidth(size.x); // Can only set width, ignore height

            std::optional<std::string> res = std::nullopt;

            if (ImGui::BeginCombo(label.c_str(), nullptr)) {
                for (auto& opt : options) {
                    if (ImGui::Selectable(opt.first.c_str())) {
                        res = opt.second;
                    }
                }
                ImGui::EndCombo();
            }

            return res;
        },

        .input_pin = [](RenderContext& ctx, ImVec2 pos, float size, NodePlot::InputId id) -> bool {
            bool updated = false;
            auto win_pos = ImGui::GetWindowPos();
            ImGui::GetForegroundDrawList()->AddCircle({win_pos.x + pos.x + size, win_pos.y + pos.y + size}, size, ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.3f, 1.0f)), 0);
            ImGui::SetCursorPos(pos);
            if (ImGui::InvisibleButton("##input_pin", {size * 2, size * 2})) {

                std::optional<NodePlot::Data> default_val;
                {
                    auto inputs = ctx.node.inputs(ctx.npf, ctx.eng, ctx.node_id);
                    if (inputs.has_value()) {
                        for (auto& i : inputs.value()) {
                            if (i.first == id) {
                                default_val = i.second.default_value;
                            }
                        }
                    }
                }

                if (default_val.has_value()) {
                    ctx.node_storage.input_storage[id] = default_val.value();
                } else {
                    ctx.node_storage.input_storage.erase(id);
                }

                updated = true;
            }
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PIN")) {
                    IM_ASSERT(payload->DataSize == sizeof(std::pair<NodePlot::NodeId, NodePlot::OutputId>));
                    std::pair<NodePlot::NodeId, NodePlot::OutputId> pin = *(const decltype(pin)*)payload->Data;

                    ctx.node_storage.input_storage[id] = NodePlot::NodeGraph::InputPin{.node_id = pin.first, .output_id = pin.second};

                    updated = true;
                }

                ImGui::EndDragDropTarget();
            }
            return updated;
        },
        .output_pin = [](RenderContext& ctx, ImVec2 pos, float size, NodePlot::OutputId id) -> bool {
            auto win_pos = ImGui::GetWindowPos();
            ImGui::GetForegroundDrawList()->AddCircle({win_pos.x + pos.x + size, win_pos.y + pos.y + size}, size, ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.3f, 1.0f)), 0);
            ImGui::SetCursorPos(pos);
            ImGui::InvisibleButton("##output_pin", {size * 2, size * 2});
            if (ImGui::BeginDragDropSource()) {
                std::pair<NodePlot::NodeId, NodePlot::OutputId> pin = {ctx.node_id, id};
                ImGui::SetDragDropPayload("PIN", &pin, sizeof(pin));
                ctx.node_renderer.draw_node_path({pos.x + win_pos.x + size, pos.y + win_pos.y + size}, ImGui::GetIO().MousePos);
                ImGui::EndDragDropSource();
                return true;
            }
            return false;
        },

        .string_input = [](RenderContext&, ImVec2 pos, ImVec2 size, std::string& storage, bool enabled) -> bool {
            ImGui::SetCursorPos(pos);
            if (!enabled)
                ImGui::BeginDisabled(true);
            ImGui::SetNextItemWidth(size.x);
            bool res = ImGui::InputText("##string_input", &storage, ImGuiInputTextFlags_None);
            if (!enabled)
                ImGui::EndDisabled();
            return res;
        },
        .boolean_input = [](RenderContext&, ImVec2 pos, bool& storage, bool enabled) -> bool {
            ImGui::SetCursorPos(pos);
            if (!enabled)
                ImGui::BeginDisabled(true);
            bool res = ImGui::Checkbox("##boolean_input", &storage);
            if (!enabled)
                ImGui::EndDisabled();
            return res;
        },
        .number_input = [](RenderContext&, ImVec2 pos, ImVec2 size, double& storage, bool enabled) -> bool {
            ImGui::SetCursorPos(pos);
            if (!enabled)
                ImGui::BeginDisabled(true);
            ImGui::SetNextItemWidth(size.x);
            bool res = ImGui::InputDouble("##number_input", &storage, ImGuiInputTextFlags_None);
            if (!enabled)
                ImGui::EndDisabled();
            return res;
        },
        .integer_input = [](RenderContext&, ImVec2 pos, ImVec2 size, int64_t& storage, bool enabled) -> bool {
            ImGui::SetCursorPos(pos);
            if (!enabled)
                ImGui::BeginDisabled(true);
            ImGui::SetNextItemWidth(size.x);
            bool res = ImGui::InputScalar("##number_input", ImGuiDataType_S64, &storage);
            if (!enabled)
                ImGui::EndDisabled();
            return res;
        },
        .color_input = [](RenderContext&, ImVec2 pos, ImVec2 size, NodePlot::Color& storage, bool enabled) -> bool {
            ImGui::SetCursorPos(pos);
            if (!enabled)
                ImGui::BeginDisabled(true);
            ImGui::SetNextItemWidth(size.x);
            bool res = ImGui::ColorEdit4("##number_input", (float*)&storage);
            if (!enabled)
                ImGui::EndDisabled();
            return res;
        },
        .margin_input = [](RenderContext&, ImVec2 pos, ImVec2 size, NodePlot::Margins& storage, bool enabled) -> bool {
            ImGui::SetCursorPos(pos);
            if (!enabled)
                ImGui::BeginDisabled(true);
            ImGui::SetNextItemWidth(size.x);
            bool res = ImGui::InputFloat4("##number_input", (float*)&storage);
            if (!enabled)
                ImGui::EndDisabled();
            return res;
        },
        .position_input = [](RenderContext&, ImVec2 pos, ImVec2 size, NodePlot::Pos& storage, bool enabled) -> bool {
            ImGui::SetCursorPos(pos);
            if (!enabled)
                ImGui::BeginDisabled(true);
            ImGui::SetNextItemWidth(size.x);
            bool res = ImGui::InputFloat2("##number_input", (float*)&storage);
            if (!enabled)
                ImGui::EndDisabled();
            return res;
        },
    };

    auto node = NodePlot::NodeRegistry::node_map.find(storage.type_id);
    if (node == NodePlot::NodeRegistry::node_map.end()) {
        ImGui::Text("Error: Invalid node type id");
        return ERR("Invalid node type id");
        return false;
    }

    RenderContext ctx{
        .node_renderer = *this,
        .npf = npf,
        .eng = eng,
        .node_id = node_id,
        .node_storage = storage,
        .node = node->second,
    };

    auto rnf = render_override_map.find(storage.type_id);
    if (rnf == render_override_map.end()) {
        return TRY(default_renderer(imgui_renderer, ctx));
    } else {
        return TRY((rnf->second)(imgui_renderer, ctx));
    }
}

ErrorOr<void> NodeRenderer::render_input_paths(NodePlot::NodeId node_id, NodePlot::NodeGraph::NodeStorage& storage) {
    Renderer input_pin_renderer{
        .input_pin = [&](RenderContext& ctx, ImVec2 pos, float size, NodePlot::InputId id) -> bool {
            auto input = ctx.node_storage.input_storage.find(id);
            if (input == ctx.node_storage.input_storage.end())
                return false;
            if (std::holds_alternative<NodePlot::NodeGraph::InputPin>(input->second)) {
                pos.x += size;
                pos.y += size;
                auto& pin = std::get<NodePlot::NodeGraph::InputPin>(input->second);

                auto output_ng = eng->node_graph(ctx.npf);
                if (!output_ng.has_value())
                    return false;

                auto output_node_storage = output_ng.value().get().nodes.find(pin.node_id);
                if (output_node_storage == output_ng.value().get().nodes.end())
                    return false;

                std::optional<ImVec2> found_pos = std::nullopt;

                Renderer output_pin_renderer{
                    .output_pin =
                        [&](RenderContext& ctx, ImVec2 pos, float size, NodePlot::OutputId id) {
                            if (id == pin.output_id)
                                found_pos = {pos.x + size, pos.y + size};
                            return false;
                        },
                };

                auto output_node = NodePlot::NodeRegistry::node_map.find(output_node_storage->second.type_id);
                if (output_node == NodePlot::NodeRegistry::node_map.end()) {
                    return false;
                }

                RenderContext output_ctx{
                    .node_renderer = *this,
                    .npf = npf,
                    .eng = eng,
                    .node_id = pin.node_id,
                    .node_storage = output_node_storage->second,
                    .node = output_node->second,
                };

                auto rnf = render_override_map.find(output_node_storage->second.type_id);
                if (rnf == render_override_map.end()) {
                    MAYBE(default_renderer(output_pin_renderer, output_ctx));
                } else {
                    MAYBE((rnf->second)(output_pin_renderer, output_ctx));
                }

                if (found_pos.has_value()) {
                    constexpr ImVec4 color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                    const ImU32 color_32 = ImGui::GetColorU32(color);

                    ImVec2 input_node_pos = ctx.node_renderer.world_to_screen({storage.pos.x, storage.pos.y});
                    ImVec2 output_node_pos = ctx.node_renderer.world_to_screen({output_node_storage->second.pos.x, output_node_storage->second.pos.y});

                    ctx.node_renderer.draw_node_path(
                        {
                            found_pos->x + output_node_pos.x,
                            found_pos->y + output_node_pos.y,
                        },
                        {
                            pos.x + input_node_pos.x,
                            pos.y + input_node_pos.y,
                        });
                }
            }

            return false;
        },
    };

    auto node = NodePlot::NodeRegistry::node_map.find(storage.type_id);
    if (node == NodePlot::NodeRegistry::node_map.end()) {
        return ERR("Invalid node type id");
        return {};
    }

    RenderContext ctx{
        .node_renderer = *this,
        .npf = npf,
        .eng = eng,
        .node_id = node_id,
        .node_storage = storage,
        .node = node->second,
    };

    auto rnf = render_override_map.find(storage.type_id);
    if (rnf == render_override_map.end()) {
        TRY(default_renderer(input_pin_renderer, ctx));
    } else {
        TRY((rnf->second)(input_pin_renderer, ctx));
    }

    return {};
}

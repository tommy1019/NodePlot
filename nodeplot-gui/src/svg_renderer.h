#pragma once

#include <memory>

#include <imgui.h>

#include <nodeplot/error.h>

struct SVGRenderer {
    static ErrorOr<std::shared_ptr<SVGRenderer>> create(ImVec2 size, std::string svg);

    ~SVGRenderer() {}

    ErrorOr<void> recreate_texture();

    ErrorOr<void> set_size(ImVec2 size) {
        auto old_size = m_size;
        m_size = size;
        if (m_size.x != old_size.x || m_size.y != old_size.y)
            return recreate_texture();
        return {};
    }

    ErrorOr<void> set_svg(std::string svg) {
        m_svg = svg;
        return recreate_texture();
    }

    void draw();

  private:
    SVGRenderer() = default;

    ImVec2 m_size = {-1, -1};
    ImVec2 m_bitmap_size = {};
    std::string m_svg;

    unsigned int m_texture = -1;
};
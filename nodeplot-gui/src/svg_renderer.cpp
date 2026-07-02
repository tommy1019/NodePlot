#include "svg_renderer.h"

#include <lunasvg.h>

#include <SDL_opengl.h>

#include <imgui_impl_opengl3.h>

ErrorOr<std::shared_ptr<SVGRenderer>> SVGRenderer::create(ImVec2 size, std::string svg) {
    auto res = std::shared_ptr<SVGRenderer>(new SVGRenderer);

    res->m_size = size;
    res->m_svg = svg;

    TRY(res->recreate_texture());

    return res;
}

ErrorOr<void> SVGRenderer::recreate_texture() {

    auto document = lunasvg::Document::loadFromData(m_svg);
    if (document == nullptr) {
        return ERR("Failed to render SVG");
    }

    ImVec2 intrinsic_size = {document->width(), document->height()};

    float scale = std::min(m_size.x / intrinsic_size.x, m_size.y / intrinsic_size.y);

    auto bitmap = document->renderToBitmap(intrinsic_size.x * scale, intrinsic_size.y * scale);
    bitmap.convertToRGBA();

    m_bitmap_size = {(float)bitmap.width(), (float)bitmap.height()};

    auto old_texture = m_texture;

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap.width(), bitmap.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.data());

    glDeleteTextures(1, &old_texture);

    return {};
}

void SVGRenderer::draw() {
    ImVec2 target_size = ImGui::GetContentRegionAvail();
    float x = (target_size.x - m_bitmap_size.x) * 0.5f;
    float y = (target_size.y - m_bitmap_size.y) * 0.5f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y);

    ImGui::Image((ImTextureID)(intptr_t)m_texture, ImVec2(m_bitmap_size.x, m_bitmap_size.y));
}

#include "window.h"

#include <memory>

DetailedErrorOr<void> Window::global_init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
        return ERR(std::string("Could not init SDL: ") + SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    IMGUI_CHECKVERSION();

    return {};
}
DetailedErrorOr<void> Window::global_deinit() {
    SDL_Quit();
    return {};
}
DetailedErrorOr<std::shared_ptr<Window>> Window::create(CreateWindowParams params) {
    std::shared_ptr<Window> res(new Window);

    res->m_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);

    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    res->m_window = SDL_CreateWindow("NodePlot", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(params.width * res->m_scale), (int)(params.height * res->m_scale), window_flags);

    res->m_gl_context = SDL_GL_CreateContext(res->m_window);
    if (res->m_gl_context == nullptr)
        return ERR(std::string("Could not create gl context: ") + SDL_GetError());

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(res->m_scale);
    style.FontScaleDpi = res->m_scale;

    const char* glsl_version = "#version 150";
    ImGui_ImplSDL2_InitForOpenGL(res->m_window, res->m_gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return res;
}
Window::~Window() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(m_gl_context);
    SDL_DestroyWindow(m_window);
}

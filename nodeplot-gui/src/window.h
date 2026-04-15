#include <nodeplot/error.h>

#include <SDL_opengl.h>

#include <SDL.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

struct Window {
  public:
    static ErrorOr<void> global_init();
    static ErrorOr<void> global_deinit();

    struct CreateWindowParams {
        size_t width = 800;
        size_t height = 600;
    };

    static ErrorOr<std::shared_ptr<Window>> create(CreateWindowParams params);

    ~Window();

    struct RenderEvent {};

    ErrorOr<void> event_loop(auto event_handler) {
        ImGuiIO& io = ImGui::GetIO();

        while (!m_close_requested) {

            {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    ImGui_ImplSDL2_ProcessEvent(&event);
                    if (event.type == SDL_QUIT)
                        m_close_requested = true;
                    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_window))
                        m_close_requested = true;
                }
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            ErrorOr<void> res = event_handler(RenderEvent{});
            if (!res.has_value())
                return res;

            ImGui::Render();
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(m_clear_color.x * m_clear_color.w, m_clear_color.y * m_clear_color.w, m_clear_color.z * m_clear_color.w, m_clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(m_window);
        }

        return {};
    }

    ErrorOr<void> set_title(std::string title) {
        SDL_SetWindowTitle(m_window, title.c_str());
        return {};
    }

    void request_close() { m_close_requested = true; }
    void set_clear_color(ImVec4 color) { m_clear_color = color; }

  private:
    Window() = default;

    SDL_Window* m_window;
    SDL_GLContext m_gl_context;

    float m_scale;
    bool m_close_requested = false;
    ImVec4 m_clear_color = ImVec4(236.0f / 255.0f, 236.0f / 255.0f, 236.0f / 255.0f, 1.00f);
};
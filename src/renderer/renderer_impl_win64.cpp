#include "renderer_impl_win64.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"

// @NOTE: Must be imported in this order
#include "glad/glad.h"
#include "GLFW/glfw3.h"
////////////////////////////////////////

#include "../btzc_game_engine.h"
#include "renderer.h"

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::stringstream;
using std::vector;


renderer::Renderer::Impl::Impl(string const& title)
{
    setup_glfw_and_opengl46_hints();

    calc_ideal_standard_window_dim_and_apply_center_hints();
    create_window_with_gfx_context(title);

    setup_imgui();
}

renderer::Renderer::Impl::~Impl()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(m_window_handle));
    glfwTerminate();
}

bool renderer::Renderer::Impl::get_requesting_close()
{
    return glfwWindowShouldClose(reinterpret_cast<GLFWwindow*>(m_window_handle));
}

void renderer::Renderer::Impl::poll_events()
{
    glfwPollEvents();
}

void renderer::Renderer::Impl::render()
{
    // Simple clear color.
    glViewport(0, 0, m_window_dims.width, m_window_dims.height);
    glClearColor(0.063f, 0.129f, 0.063f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    render_imgui();

    glfwSwapBuffers(reinterpret_cast<GLFWwindow*>(m_window_handle));
}

void renderer::Renderer::Impl::setup_glfw_and_opengl46_hints()
{
    // Init glfw and create main window.
    auto result = glfwInit();
    assert(result == GLFW_TRUE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

#if defined(__APPLE__)
    // @NOTE: Only for MacOS.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

void renderer::Renderer::Impl::calc_ideal_standard_window_dim_and_apply_center_hints()
{
    static vector<Window_dimensions> const k_standard_window_dims{
        { 3840, 2160 },
        { 2560, 1440 },
        { 1920, 1080 },
        { 1280, 720 },
        { 1024, 576 },
        { 800, 450 },
        { 640, 360 },
    };

    // calc_ideal_std_window_dim()
    struct Monitor_workarea
    {
        int32_t xpos;
        int32_t ypos;
        int32_t width;
        int32_t height;
    } monitor_workarea;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(),
                           &monitor_workarea.xpos,
                           &monitor_workarea.ypos,
                           &monitor_workarea.width,
                           &monitor_workarea.height);

    // Find largest dimension within monitor workarea.
    Window_dimensions ideal_std_win_dim{ 0, 0 };
    for (auto& window_dim : k_standard_window_dims)
    {
        if (window_dim.width < monitor_workarea.width &&
            window_dim.height < monitor_workarea.height)
        {
            ideal_std_win_dim = window_dim;
            break;
        }
    }

    assert(ideal_std_win_dim.width > 0 &&
           ideal_std_win_dim.height > 0);
    m_window_dims = ideal_std_win_dim;

    // Apply centering hints.
    int32_t centered_window_pos[2]{
        monitor_workarea.xpos
            + static_cast<int32_t>(monitor_workarea.width * 0.5
                - m_window_dims.width * 0.5),
        monitor_workarea.ypos
            + static_cast<int32_t>(monitor_workarea.height * 0.5
                - m_window_dims.height * 0.5),
    };

    glfwWindowHint(GLFW_POSITION_X, centered_window_pos[0]);
    glfwWindowHint(GLFW_POSITION_Y, centered_window_pos[1]);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
}

// GLFW window callbacks.
namespace bt_glfw_window_callbacks
{

static void key_callback(GLFWwindow* window,
                         int32_t key,
                         int32_t scancode,
                         int32_t action,
                         int32_t mods)
{
}

static void mouse_button_callback(GLFWwindow* window,
                                  int32_t button,
                                  int32_t action,
                                  int32_t mods)
{
}

static void cursor_position_callback(GLFWwindow* window,
                                     double_t xpos,
                                     double_t ypos)
{
}

static void scroll_callback(GLFWwindow* window,
                            double_t xoffset,
                            double_t yoffset)
{
}

static void window_focus_callback(GLFWwindow* window,
                                  int32_t focused)
{
}

static void window_iconify_callback(GLFWwindow* window,
                                    int32_t iconified)
{
}

static void window_resize_callback(GLFWwindow* window,
                                   int32_t width,
                                   int32_t height)
{
}

}  // namespace bt_glfw_window_callbacks

void renderer::Renderer::Impl::create_window_with_gfx_context(string const& title)
{
    stringstream full_title_str;
    full_title_str
        << title                      << " - "
        << BTZC_GAME_ENGINE_DEV_STAGE << " - "
        << BTZC_GAME_ENGINE_VERSION   << " - "
        << BTZC_GAME_ENGINE_OS_NAME   << " - "
        << "OpenGL 4.6 (Compatibility mode)";

    m_window_handle = glfwCreateWindow(m_window_dims.width,
                                       m_window_dims.height,
                                       full_title_str.str().c_str(),
                                       nullptr,
                                       nullptr);
    assert(m_window_handle != nullptr);

    // Window callbacks.
    // @NOTE: With key callbacks etc that's also used by Imgui, Imgui
    //   chains these callbacks so they don't get lost.
    auto win_handle{ reinterpret_cast<GLFWwindow*>(m_window_handle) };
    glfwSetKeyCallback(win_handle, bt_glfw_window_callbacks::key_callback);
    glfwSetMouseButtonCallback(win_handle, bt_glfw_window_callbacks::mouse_button_callback);
    glfwSetCursorPosCallback(win_handle, bt_glfw_window_callbacks::cursor_position_callback);
    glfwSetScrollCallback(win_handle, bt_glfw_window_callbacks::scroll_callback);
    glfwSetWindowFocusCallback(win_handle, bt_glfw_window_callbacks::window_focus_callback);
    glfwSetWindowIconifyCallback(win_handle, bt_glfw_window_callbacks::window_iconify_callback);
    glfwSetWindowSizeCallback(win_handle, bt_glfw_window_callbacks::window_resize_callback);

    // Config and load OpenGL context.
    glfwMakeContextCurrent(win_handle);
    glfwSwapInterval(1);  // Vsync on.

    gladLoadGL();
}

void renderer::Renderer::Impl::setup_imgui()
{
    // Setup Dear ImGui context.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls.
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking.
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows.

    // Setup Dear ImGui style.
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform
    // windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends.
    ImGui_ImplGlfw_InitForOpenGL(reinterpret_cast<GLFWwindow*>(m_window_handle), true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

void renderer::Renderer::Impl::render_imgui()
{
    // @NOCHECKIN: @TEMP
    static bool show_demo_window = true;
    static ImGuiIO& io = ImGui::GetIO();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    }
    ImGui::End();

    // Rendering.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows.
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    // @CHECK: @THEA: Idk why this is here but here's some notes ^^^
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

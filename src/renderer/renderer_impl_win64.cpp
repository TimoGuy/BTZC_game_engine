#include "renderer_impl_win64.h"

#include "fmt/base.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"

// @NOTE: Must be imported in this order
#include "glad/glad.h"
#include "GLFW/glfw3.h"
////////////////////////////////////////

#include "../btzc_game_engine.h"
#include "../input_handler/input_handler.h"
#include "cglm/cglm.h"
#include "renderer.h"

#include <cassert>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

using std::mutex;
using std::lock_guard;
using std::string;
using std::stringstream;
using std::vector;


namespace
{

// Helper pointers for GLFW callbacks.
GLFWwindow* s_main_window{ nullptr };
BT::Renderer::Impl* s_main_renderer{ nullptr };

// GLFW window callbacks.
static void key_callback(GLFWwindow* window,
                         int32_t key,
                         int32_t scancode,
                         int32_t action,
                         int32_t mods)
{
    assert(window == s_main_window);
    switch (action)
    {
    case GLFW_PRESS:
    case GLFW_RELEASE:
        s_main_renderer->get_input_handler().report_keyboard_input_change(
            key,
            (action == GLFW_PRESS ? true : false));
        break;
    }
}

static void mouse_button_callback(GLFWwindow* window,
                                  int32_t button,
                                  int32_t action,
                                  int32_t mods)
{
    assert(window == s_main_window);
    switch (action)
    {
    case GLFW_PRESS:
    case GLFW_RELEASE:
        s_main_renderer->get_input_handler().report_mouse_button_input_change(
            button,
            (action == GLFW_PRESS ? true : false));
        break;
    }
}

static void cursor_position_callback(GLFWwindow* window,
                                     double_t xpos,
                                     double_t ypos)
{
    assert(window == s_main_window);
    s_main_renderer->get_input_handler().report_mouse_position_change(xpos, ypos);
}

static void scroll_callback(GLFWwindow* window,
                            double_t xoffset,
                            double_t yoffset)
{
    assert(window == s_main_window);
    s_main_renderer->get_input_handler().report_mouse_scroll_input_change(xoffset, yoffset);
}

static void window_focus_callback(GLFWwindow* window,
                                  int32_t focused)
{
    assert(window == s_main_window);
    if (focused == GLFW_TRUE || focused == GLFW_FALSE)
    {
        s_main_renderer->submit_window_focused(focused == GLFW_TRUE);
    }
    else
    {
        // Invalid state.
        assert(false);
    }
}

static void window_iconify_callback(GLFWwindow* window,
                                    int32_t iconified)
{
    assert(window == s_main_window);
    if (iconified == GLFW_TRUE || iconified == GLFW_FALSE)
    {
        s_main_renderer->submit_window_iconified(iconified == GLFW_TRUE);
    }
    else
    {
        // Invalid state.
        assert(false);
    }
}

static void window_resize_callback(GLFWwindow* window,
                                   int32_t width,
                                   int32_t height)
{
    assert(window == s_main_window);
    if (width > 0 && height > 0)
    {
        s_main_renderer->submit_window_dims(width, height);
    }
}

}  // namespace


BT::Renderer::Impl::Impl(Input_handler& input_handler, string const& title)
    : m_input_handler{ input_handler }
{
    static mutex s_renderer_creation_mutex;
    lock_guard lock{ s_renderer_creation_mutex };

    static bool s_created{ false };
    assert(!s_created);

    setup_glfw_and_opengl46_hints();

    calc_ideal_standard_window_dim_and_apply_center_hints();
    create_window_with_gfx_context(title);

    setup_imgui();
    calc_3d_aspect_ratio();
    create_hdr_fbo();

    s_main_window = reinterpret_cast<GLFWwindow*>(m_window_handle);
    s_main_renderer = this;

    s_created = true;
}

BT::Renderer::Impl::~Impl()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(m_window_handle));
    glfwTerminate();
}

bool BT::Renderer::Impl::get_requesting_close()
{
    return glfwWindowShouldClose(reinterpret_cast<GLFWwindow*>(m_window_handle));
}

void BT::Renderer::Impl::poll_events()
{
    glfwPollEvents();
}

void BT::Renderer::Impl::render()
{
    if (m_window_dims_changed)
    {
        // Recreate window dimension-dependent resources.
        calc_3d_aspect_ratio();
        create_hdr_fbo();
        m_window_dims_changed = false;
    }

    // Render new frame.
    begin_new_display_frame();
    render_scene_to_hdr_framebuffer();
    render_hdr_color_to_display_frame();
    render_imgui();

    glfwSwapBuffers(reinterpret_cast<GLFWwindow*>(m_window_handle));
}

void BT::Renderer::Impl::submit_window_focused(bool focused)
{
    m_window_focused = focused;
}

void BT::Renderer::Impl::submit_window_iconified(bool iconified)
{
    m_window_iconified = iconified;
}

void BT::Renderer::Impl::submit_window_dims(int32_t width, int32_t height)
{
    m_window_dims.width = width;
    m_window_dims.height = height;
    m_window_dims_changed = true;
}

void BT::Renderer::Impl::setup_glfw_and_opengl46_hints()
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

void BT::Renderer::Impl::calc_ideal_standard_window_dim_and_apply_center_hints()
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
    submit_window_dims(ideal_std_win_dim.width, ideal_std_win_dim.height);

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

void BT::Renderer::Impl::create_window_with_gfx_context(string const& title)
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
    glfwSetKeyCallback(win_handle, key_callback);
    glfwSetMouseButtonCallback(win_handle, mouse_button_callback);
    glfwSetCursorPosCallback(win_handle, cursor_position_callback);
    glfwSetScrollCallback(win_handle, scroll_callback);
    glfwSetWindowFocusCallback(win_handle, window_focus_callback);
    glfwSetWindowIconifyCallback(win_handle, window_iconify_callback);
    glfwSetWindowSizeCallback(win_handle, window_resize_callback);

    // Config and load OpenGL context.
    glfwMakeContextCurrent(win_handle);
    glfwSwapInterval(1);  // Vsync on.

    gladLoadGL();
    glEnable(GL_DEPTH_TEST);
}

// ImGui.
void BT::Renderer::Impl::setup_imgui()
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

void BT::Renderer::Impl::render_imgui()
{
    // @NOCHECKIN: @TEMP
    static bool show_demo_window = true;
    static ImGuiIO& io = ImGui::GetIO();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main menu bar.
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Menu"))
        {
            if (ImGui::MenuItem("New")) {}
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Main dockspace.
    ImGui::DockSpaceOverViewport(0,
                                 ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
    {
        static float_t f = 0.0f;
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

// 3D camera.
void BT::Renderer::Impl::setup_3d_camera()
{
    m_camera = {
        glm_rad(90.0f),
        static_cast<float_t>(0xCAFEBABE),  // Dummy value.
        0.1f,
        500.0f,
    };
    calc_3d_aspect_ratio();
}

void BT::Renderer::Impl::calc_3d_aspect_ratio()
{
    m_camera.aspect_ratio =
        static_cast<float_t>(m_window_dims.width)
            / static_cast<float_t>(m_window_dims.height);
}

void BT::Renderer::Impl::calc_camera_matrices(mat4& out_projection, mat4& out_view, mat4& out_projection_view)
{
    // Calculate projection matrix.
    glm_perspective(m_camera.fov,
                    m_camera.aspect_ratio,
                    m_camera.z_near,
                    m_camera.z_far,
                    out_projection);
    // out_projection[1][1] *= -1.0f;  // @NOTE: Vulkan only.

    // Calculate view matrix.
    using std::abs;
    vec3 up{ 0.0f, 1.0f, 0.0f };
    if (abs(m_camera.view_direction[0]) < 1e-6f &&
        abs(m_camera.view_direction[1]) > 1e-6f &&
        abs(m_camera.view_direction[2]) < 1e-6f)
    {
        glm_vec3_copy(vec3{ 0.0f, 0.0f, 1.0f }, up);
    }

    vec3 center;
    glm_vec3_add(m_camera.position, m_camera.view_direction, center);
    glm_lookat(m_camera.position, center, up, out_view);

    // Calculate projection view matrix.
    glm_mat4_mul(out_projection,
                 out_view,
                 out_projection_view);
}

// Display rendering.
void BT::Renderer::Impl::begin_new_display_frame()
{
    // Configure main render target.
    glViewport(0, 0, m_window_dims.width, m_window_dims.height);
    glClearColor(0.063f, 0.129f, 0.063f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void BT::Renderer::Impl::render_hdr_color_to_display_frame()
{
    // Render hdr framebuffer to main render target.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use tonemapping shader.
    // @TODO

    render_ndc_quad();
}

// HDR rendering.
void BT::Renderer::Impl::create_hdr_fbo()
{
    if (m_hdr_fbo != 0 || m_hdr_color_texture != 0 || m_hdr_depth_rbo != 0)
    {
        // Double check that everything is fully created prior to deleting to recreate.
        assert(m_hdr_fbo != 0 && m_hdr_color_texture != 0 && m_hdr_depth_rbo != 0);

        // Delete to recreate.
        glDeleteFramebuffers(1, &m_hdr_fbo);
        glDeleteTextures(1, &m_hdr_color_texture);
        glDeleteRenderbuffers(1, &m_hdr_depth_rbo);
    }

    // Create color texture.
    glGenTextures(1, &m_hdr_color_texture);
    glBindTexture(GL_TEXTURE_2D, m_hdr_color_texture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA16F,
                 m_window_dims.width,
                 m_window_dims.height,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create depth renderbuffer.
    glGenRenderbuffers(1, &m_hdr_depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_hdr_depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER,
                          GL_DEPTH_COMPONENT,
                          m_window_dims.width,
                          m_window_dims.height);

    // Create framebuffer.
    glGenFramebuffers(1, &m_hdr_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdr_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           m_hdr_color_texture,
                           0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              m_hdr_depth_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fmt::println("ERROR: Framebuffer incomplete.");
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BT::Renderer::Impl::render_scene_to_hdr_framebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdr_fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calc camera.
    mat4 projection;
    mat4 view;
    mat4 projection_view;
    calc_camera_matrices(projection, view, projection_view);

    // Use rendering shader.
    // @TODO.

    // Render scene.
    // @TODO.

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Helper functions.
void BT::Renderer::Impl::render_ndc_cube()
{
    static uint32_t vao{ 0 };
    static uint32_t vbo{ 0 };

    if (vao == 0)
    {
        // Init mesh.
        float_t vertices[]{
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        // Fill buffer.
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Link vertex attributes.
        glBindVertexArray(vao);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              8 * sizeof(float_t),
                              reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              8 * sizeof(float_t),
                              reinterpret_cast<void*>(3 * sizeof(float_t)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              8 * sizeof(float_t),
                              reinterpret_cast<void*>(6 * sizeof(float_t)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Render cube.
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void BT::Renderer::Impl::render_ndc_quad()
{
    static uint32_t vao{ 0 };
    static uint32_t vbo{ 0 };

    if (vao == 0)
    {
        // Init mesh.
        float_t vertices[]{
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };

        // Setup plane VAO.
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              5 * sizeof(float_t),
                              reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              5 * sizeof(float_t),
                              reinterpret_cast<void*>(3 * sizeof(float_t)));
    }

    // Render quad.
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

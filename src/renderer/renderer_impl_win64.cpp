#include "renderer_impl_win64.h"

#include "cglm/mat4.h"
#include "cglm/vec3.h"
#include "cglm/cglm.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuizmo.h"

// @NOTE: Must be imported in this order
#include "glad/glad.h"
#include "GLFW/glfw3.h"
////////////////////////////////////////

#include "../btzc_game_engine.h"
#include "../input_handler/input_handler.h"
#include "../game_object/game_object.h"
#include "logger.h"
#include "material.h"
#include "render_object.h"
#include "renderer.h"
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#include "texture.h"
#include <cassert>
#include <gl/gl.h>
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


BT::Renderer::Impl::Impl(Renderer& renderer, ImGui_renderer& imgui_renderer, Input_handler& input_handler, string const& title)
    : m_renderer{ renderer }
    , m_imgui_renderer{ imgui_renderer }
    , m_input_handler{ input_handler }
{
    static mutex s_renderer_creation_mutex;
    lock_guard lock{ s_renderer_creation_mutex };

    static bool s_created{ false };
    assert(!s_created);

    setup_glfw_and_opengl46_hints();

    calc_ideal_standard_window_dim_and_apply_center_hints();
    create_window_with_gfx_context(title);

    setup_imgui();
    create_ldr_fbo();
    create_hdr_fbo();

    m_camera.set_callbacks(
        [&](bool lock) {
            // Enable/disable ImGui reading mouse and keyboard interactions.
            if (lock)  ImGui::GetIO().ConfigFlags |= (ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard);
            else       ImGui::GetIO().ConfigFlags &= ~(ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard);

            glfwSetInputMode(reinterpret_cast<GLFWwindow*>(m_window_handle),
                             GLFW_CURSOR,
                             (lock ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL));
        });
    m_camera.set_aspect_ratio(m_main_viewport_dims.width,
                              m_main_viewport_dims.height);

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

void BT::Renderer::Impl::render(float_t delta_time, function<void()>&& debug_views_render_fn)
{
    if (m_main_viewport_wanted_dims.width != m_main_viewport_dims.width ||
        m_main_viewport_wanted_dims.height != m_main_viewport_dims.height)
    {
        // Recreate window dimension-dependent resources.
        m_main_viewport_dims = m_main_viewport_wanted_dims;
        create_ldr_fbo();
        create_hdr_fbo();
        m_camera.set_aspect_ratio(m_main_viewport_dims.width,
                                  m_main_viewport_dims.height);
    }

    // Update camera.
    m_camera.update_frontend(m_renderer, m_input_handler.get_input_state(), delta_time);
    m_camera.update_camera_matrices();

    // Render new frame.
    begin_new_display_frame();
    render_scene_to_hdr_framebuffer();
    render_hdr_color_to_ldr_framebuffer();
    render_debug_views_to_ldr_framebuffer(std::move(debug_views_render_fn));
    render_imgui();

    present_display_frame();

    m_input_handler.clear_look_delta();
    // m_input_handler.clear_ui_scroll_delta();  @UNSURE
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

    if (!m_render_to_ldr)
    {
        m_main_viewport_wanted_dims = m_window_dims;
    }
}

void BT::Renderer::Impl::fetch_cached_camera_matrices(mat4& out_projection,
                                                      mat4& out_view,
                                                      mat4& out_projection_view)
{
    m_camera.fetch_calculated_camera_matrices(out_projection, out_view, out_projection_view);
}

BT::Camera* BT::Renderer::Impl::get_camera_obj()
{
    return &m_camera;
}

BT::Render_object_pool& BT::Renderer::Impl::get_render_object_pool()
{
    return m_rend_obj_pool;
}

void BT::Renderer::Impl::render_imgui_game_view()
{
    ImVec2 content_size{ ImGui::GetContentRegionAvail() };
    ImGui::ImageWithBg(m_ldr_color_texture, content_size);

    bool is_item_hovered{ ImGui::IsItemHovered() };
    m_camera.set_hovering_over_game_viewport(is_item_hovered);

    if (is_item_hovered || m_camera.is_mouse_captured())
    {
        // Communicate that no capture mouse wanted when hovering over this viewport.
        ImGui::SetNextFrameWantCaptureMouse(false);
    }

    if (m_render_to_ldr)
    {
        m_main_viewport_wanted_dims.width = content_size.x;
        m_main_viewport_wanted_dims.height = content_size.y;
    }

    // Set ImGuizmo draw bounds.
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    auto rect_min{ ImGui::GetItemRectMin() };
    auto rect_size{ ImGui::GetItemRectSize() };
    ImGuizmo::SetRect(rect_min.x, rect_min.y, rect_size.x, rect_size.y);
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
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // Apply wanted viewport dimensions.
    if (!m_render_to_ldr)
    {
        m_main_viewport_wanted_dims = m_window_dims;
    }
}

void BT::Renderer::Impl::create_window_with_gfx_context(string const& title)
{
    // Load window icon.
    string window_icon_fname{ BTZC_GAME_ENGINE_ASSET_SETTINGS_PATH "app_icon_48x48.png" };
    int32_t width;
    int32_t height;
    int32_t num_channels;
    stbi_set_flip_vertically_on_load_thread(false);
    uint8_t* data{ stbi_load(window_icon_fname.c_str(),
                             &width,
                             &height,
                             &num_channels,
                             STBI_default) };
    if (width != height)
    {
        logger::printef(logger::ERROR,
                        "Dimensions are unequal. Unsuitable for a window icon. Dims={ %i, %i }",
                        width, height);
        assert(false);
        return;
    }
    if (num_channels != 4)
    {
        logger::printef(logger::ERROR,
                        "Window icon \"%s\" is not RGBA. Channels: %i",
                        window_icon_fname.c_str(),
                        num_channels);
        assert(false);
        return;
    }

    // Resize to wanted icon sizes.
    // @NOTE: @TODO: FUCK GLFW!!!!!! Make your own win32 wrapper or set the win32
    //   icon selection to a different one than the shit glfw has. If I provide
    //   16x16 and a 48x48 icons for win32, glfw will select the 16x16 for the 16x16
    //   AND the 32x32 instead of the 48x48. So the big icon looks like hot garbage.
    // @NOTE: 16x16 and 48x48 icons for windows are the best sizes w/ the different
    //   sizes for windows (taskbar (esp. XL size) and decorated window).
    // @WORKAROUND: only provide the 48x48 icon. The 16x16 icon suffers but oh well.
    constexpr array<uint32_t, 1> k_wanted_icon_sizes{ 48 };
    vector<GLFWimage> window_icons;
    for (uint32_t wanted_size : k_wanted_icon_sizes)
    {
        uint8_t* output_data{
            stbir_resize_uint8_srgb(data, width, height, 0,
                                    nullptr, wanted_size, wanted_size, 0,
                                    STBIR_RGBA) };
        window_icons.emplace_back(wanted_size, wanted_size, output_data);
    }

    // Create window.
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
    auto win_handle{ reinterpret_cast<GLFWwindow*>(m_window_handle) };

    // Set window icons.
    glfwSetWindowIcon(win_handle,
                      static_cast<int>(window_icons.size()),
                      window_icons.data());
    glfwShowWindow(win_handle);

    // Window callbacks.
    // @NOTE: With key callbacks etc that's also used by Imgui, Imgui
    //   chains these callbacks so they don't get lost.
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
    // Start the Dear ImGui frame.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Start ImGuizmo frame.
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();

    // Render imgui stuff.
    m_imgui_renderer.render_imgui();

    // Rendering.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows.
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    // @CHECK: @THEA: Idk why this is here but here's some notes ^^^
    static ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

// Display rendering.
void BT::Renderer::Impl::create_ldr_fbo()  // @COPYPASTA: see `create_hdr_fbo()`.
{
    if (m_ldr_fbo != 0 || m_ldr_color_texture != 0 || m_ldr_depth_rbo != 0)
    {
        // Double check that everything is fully created prior to deleting to recreate.
        assert(m_ldr_fbo != 0 && m_ldr_color_texture != 0 && m_ldr_depth_rbo != 0);

        // Delete to recreate.
        glDeleteFramebuffers(1, &m_ldr_fbo);
        glDeleteTextures(1, &m_ldr_color_texture);
        glDeleteRenderbuffers(1, &m_ldr_depth_rbo);
    }

    // Create color texture.
    glGenTextures(1, &m_ldr_color_texture);
    glBindTexture(GL_TEXTURE_2D, m_ldr_color_texture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA8,
                 m_main_viewport_dims.width,
                 m_main_viewport_dims.height,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create depth renderbuffer.
    glGenRenderbuffers(1, &m_ldr_depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_ldr_depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER,
                          GL_DEPTH_COMPONENT,
                          m_main_viewport_dims.width,
                          m_main_viewport_dims.height);

    // Create framebuffer.
    glGenFramebuffers(1, &m_ldr_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ldr_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           m_ldr_color_texture,
                           0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              m_ldr_depth_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        logger::printef(logger::ERROR, "Framebuffer incomplete.");
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BT::Renderer::Impl::begin_new_display_frame()
{
    // Configure main render target.
    glViewport(0, 0, m_window_dims.width, m_window_dims.height);
    glClearColor(0.063f, 0.129f, 0.063f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void BT::Renderer::Impl::render_hdr_color_to_ldr_framebuffer()
{
    if (m_render_to_ldr)
    {
        // Assign ldr fbo.
        glBindFramebuffer(GL_FRAMEBUFFER, m_ldr_fbo);
    }

    // Render hdr framebuffer to main render target.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use tonemapping shader.
    static Material_ifc* s_post_process_material{ Material_bank::get_material("post_process") };
    s_post_process_material->bind_material(GLM_MAT4_ZERO);
    render_ndc_quad();
    s_post_process_material->unbind_material();

    if (m_render_to_ldr)
    {
        // Unassign ldr fbo.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void BT::Renderer::Impl::render_debug_views_to_ldr_framebuffer(function<void()>&& debug_views_render_fn)
{
    glEnable(GL_DEPTH_TEST);

    if (m_render_to_ldr)
    {
        // Assign ldr fbo.
        glBindFramebuffer(GL_FRAMEBUFFER, m_ldr_fbo);
    }

    // Copy depth buffer of hdr buffer over.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_hdr_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_render_to_ldr ? m_ldr_fbo : 0);
    glBlitFramebuffer(0, 0, m_main_viewport_dims.width, m_main_viewport_dims.height,
                      0, 0, m_main_viewport_dims.width, m_main_viewport_dims.height,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    // Render debug views.
    debug_views_render_fn();

    if (m_render_to_ldr)
    {
        // Unassign ldr fbo.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glDisable(GL_DEPTH_TEST);
}

void BT::Renderer::Impl::present_display_frame()
{
    glfwSwapBuffers(reinterpret_cast<GLFWwindow*>(m_window_handle));
}

// HDR rendering.
void BT::Renderer::Impl::create_hdr_fbo()  // @COPYPASTA.
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
                 m_main_viewport_dims.width,
                 m_main_viewport_dims.height,
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
                          m_main_viewport_dims.width,
                          m_main_viewport_dims.height);

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
        logger::printef(logger::ERROR, "Framebuffer incomplete.");
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Texture_bank::emplace_texture_2d("hdr_color_texture", m_hdr_color_texture, true);
}

void BT::Renderer::Impl::render_scene_to_hdr_framebuffer()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glBindFramebuffer(GL_FRAMEBUFFER, m_hdr_fbo);
    glViewport(0, 0, m_main_viewport_dims.width, m_main_viewport_dims.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render scene.
    auto rend_objs{ m_rend_obj_pool.checkout_all_render_objs() };
    for (auto rend_obj : rend_objs)
    {
        rend_obj->render(m_active_render_layers);
    }
    m_rend_obj_pool.return_render_objs(std::move(rend_objs));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
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

#include "camera.h"

#include "../input_handler/input_handler.h"
#include "cglm/cglm.h"
#include "imgui.h"
#include <array>
#include <memory>
#include <string>

using std::array;
using std::make_unique;
using std::string;


namespace BT
{

struct Camera::Data
{
    function<void(bool)> cursor_lock_fn;

    struct Camera_3D
    {
        float_t fov;
        float_t aspect_ratio;
        float_t z_near;
        float_t z_far;
        vec3 position{ 0.0f, 0.0f, 0.0f };
        vec3 view_direction{ 0.0f, 0.0f, 1.0f };
    } camera;

    struct Camera_matrices_cache
    {
        mat4 projection;
        mat4 view;
        mat4 projection_view;
    } camera_matrices_cache;

    struct Frontend
    {
        enum Frontend_state : uint8_t
        {
            FRONTEND_CAMERA_STATE_STATIC = 0,
            FRONTEND_CAMERA_STATE_CAPTURE_FLY,
            FRONTEND_CAMERA_STATE_FOLLOW_ORBIT,
            NUM_FRONTEND_CAMERA_STATES
        } state = FRONTEND_CAMERA_STATE_STATIC;

        inline static array<string, NUM_FRONTEND_CAMERA_STATES> s_state_strs{
            "STATIC",
            "CAPTURE FLY",
            "ORBIT",
        };

        bool prev_rclick_cam{ false };

        struct Capture_fly
        {
            float_t sensitivity{ 0.1f };
            float_t speed{ 20.0f };
        } capture_fly;
    } frontend;
};

}  // namespace BT


BT::Camera::Camera()
    : m_data(make_unique<Data>())
{
    m_data->camera = {
        glm_rad(90.0f),
        static_cast<float_t>(0xCAFEBABE),  // Dummy value.
        0.1f,
        500.0f,
    };
}

BT::Camera::~Camera() = default;  // For smart pimpl.

// Callbacks.
void BT::Camera::set_callbacks(function<void(bool)>&& cursor_lock_fn)
{
    m_data->cursor_lock_fn = std::move(cursor_lock_fn);
}

// GPU camera.
void BT::Camera::set_fov(float_t fov)
{
    m_data->camera.fov = fov;
}

void BT::Camera::set_aspect_ratio(float_t width, float_t height)
{
    m_data->camera.aspect_ratio = (width / height);
}

void BT::Camera::set_z_clip_plane(float_t z_near, float_t z_far)
{
    m_data->camera.z_near = z_near;
    m_data->camera.z_far  = z_far;
}

void BT::Camera::update_camera_matrices()
{
    auto& camera{ m_data->camera };
    auto& cache{ m_data->camera_matrices_cache };

    // Calculate projection matrix.
    glm_perspective(camera.fov,
                    camera.aspect_ratio,
                    camera.z_near,
                    camera.z_far,
                    cache.projection);
    cache.projection[1][1] *= -1.0f;  // Fix neg-Y issue.

    // Calculate view matrix.
    using std::abs;
    vec3 up{ 0.0f, 1.0f, 0.0f };
    if (abs(camera.view_direction[0]) < 1e-6f &&
        abs(camera.view_direction[1]) > 0.5f &&
        abs(camera.view_direction[2]) < 1e-6f)
    {
        glm_vec3_copy(vec3{ 0.0f, 0.0f, 1.0f }, up);
    }

    vec3 center;
    glm_vec3_add(camera.position, camera.view_direction, center);
    glm_lookat(camera.position, center, up, cache.view);

    // Calculate projection view matrix.
    glm_mat4_mul(cache.projection,
                 cache.view,
                 cache.projection_view);
}

void BT::Camera::fetch_calculated_camera_matrices(mat4& out_projection,
                                                  mat4& out_view,
                                                  mat4& out_projection_view)
{
    auto& cache{ m_data->camera_matrices_cache };
    glm_mat4_copy(cache.projection, out_projection);
    glm_mat4_copy(cache.view, out_view);
    glm_mat4_copy(cache.projection_view, out_projection_view);
}

// Camera frontend.
void BT::Camera::set_follow_object(Render_object const* render_object)
{
    // @TODO
    assert(false);
}

void BT::Camera::update_frontend(Input_handler::State const& input_state, float_t delta_time)
{
    auto& frontend{ m_data->frontend };

    // Use input state.
    bool on_press_le_rclick_cam{ !frontend.prev_rclick_cam && input_state.le_rclick_cam.val };
    bool on_release_le_rclick_cam{ frontend.prev_rclick_cam && !input_state.le_rclick_cam.val };
    frontend.prev_rclick_cam = input_state.le_rclick_cam.val;

    // Process state.
    Data::Frontend::Frontend_state prev_state;
    do
    {
        prev_state = frontend.state;

        switch (frontend.state)
        {
            case Data::Frontend::FRONTEND_CAMERA_STATE_STATIC:
                update_frontend_static();
                if (!ImGui::GetIO().WantCaptureMouse && on_press_le_rclick_cam)
                {
                    // Enter capture fly mode.
                    frontend.state = Data::Frontend::FRONTEND_CAMERA_STATE_CAPTURE_FLY;
                    m_data->cursor_lock_fn(true);
                }
                break;

            case Data::Frontend::FRONTEND_CAMERA_STATE_CAPTURE_FLY:
                update_frontend_capture_fly(input_state, delta_time);
                if (on_release_le_rclick_cam)
                {
                    // Enter static mode.
                    frontend.state = Data::Frontend::FRONTEND_CAMERA_STATE_STATIC;
                    m_data->cursor_lock_fn(false);
                }
                break;

            case Data::Frontend::FRONTEND_CAMERA_STATE_FOLLOW_ORBIT:
                update_frontend_follow_orbit();
                break;

            default:
                // Unsupported state attempted to be handled.
                assert(false);
                break;
        }
    } while (prev_state != frontend.state);
}

bool BT::Camera::is_mouse_captured()
{
    return (m_data->frontend.state ==
                Data::Frontend::FRONTEND_CAMERA_STATE_CAPTURE_FLY);
}

// ImGui.
void BT::Camera::render_imgui()
{
    auto& camera{ m_data->camera };

    ImGui::Begin("Camera properties");
    {
        ImGui::Text("Mode: %s", Data::Frontend::s_state_strs[m_data->frontend.state].c_str());
        ImGui::Text("aspect_ratio: %.3f", camera.aspect_ratio);

        float_t fov_deg{ glm_deg(camera.fov) };
        if (ImGui::DragFloat("fov (degrees)", &fov_deg, 1.0f, 1.0f, 179.0f))
        {
            camera.fov = glm_rad(fov_deg);
        }

        ImGui::DragFloat("z_near", &camera.z_near, 0.1f, 1e-6f);
        ImGui::DragFloat("z_far", &camera.z_far, 1.0f, 1e-6f);

        vec3 neg_x_cam_pos{ -camera.position[0], camera.position[1], camera.position[2] };
        if (ImGui::DragFloat3("position", neg_x_cam_pos, 0.1f))
        {
            // @NOTE: X is negated due to projection bug so this makes the editor seem okayer.
            glm_vec3_copy(neg_x_cam_pos, camera.position);
            camera.position[0] *= -1.0f;
        }

        vec3 neg_x_view_dir{ -camera.view_direction[0], camera.view_direction[1], camera.view_direction[2] };
        if (ImGui::DragFloat3("view_direction", neg_x_view_dir, 0.1f))
        {
            // @NOTE: X is negated due to projection bug so this makes the editor seem okayer.
            glm_vec3_copy(neg_x_view_dir, camera.view_direction);
            camera.view_direction[0] *= -1.0f;
        }
    }
    ImGui::End();
}

// Frontend functions.
void BT::Camera::update_frontend_static()
{
    // Do nothing.
}

void BT::Camera::update_frontend_capture_fly(Input_handler::State const& input_state, float_t delta_time)
{
    auto& camera{ m_data->camera };
    auto& capture_fly{ m_data->frontend.capture_fly };

    // Move camera with camera delta.
    vec2 cooked_cam_delta;
    glm_vec2_scale(vec2{ input_state.look_delta.x.val, input_state.look_delta.y.val },
                   capture_fly.sensitivity,
                   cooked_cam_delta);

    vec3 world_up{ 0.0f, 1.0f, 0.0f };
    vec3 world_down{ 0.0f, -1.0f, 0.0f };

    // Update camera view direction with input.
    vec3 facing_direction_right;
    glm_cross(camera.view_direction,
              world_up,
              facing_direction_right);
    glm_normalize(facing_direction_right);

    mat4 rotation = GLM_MAT4_IDENTITY_INIT;
    glm_rotate(rotation, glm_rad(-cooked_cam_delta[1]), facing_direction_right);

    vec3 new_view_direction;
    glm_mat4_mulv3(rotation,
                   camera.view_direction,
                   0.0f,
                   new_view_direction);

    if (glm_vec3_angle(new_view_direction, world_up) > glm_rad(5.0f) &&
        glm_vec3_angle(new_view_direction, world_down) > glm_rad(5.0f))
    {
        glm_vec3_copy(new_view_direction, camera.view_direction);
    }

    glm_mat4_identity(rotation);
    glm_rotate(rotation, glm_rad(-cooked_cam_delta[0]), world_up);
    glm_mat4_mulv3(rotation,
                   camera.view_direction,
                   0.0f,
                   camera.view_direction);

    // @NOTE: Need a normalization step at the end from float inaccuracy over time.
    glm_vec3_normalize(camera.view_direction);

    vec2 cooked_mvt;
    glm_vec2_scale(vec2{ input_state.move.x.val, input_state.move.y.val },
                   capture_fly.speed * delta_time,
                   cooked_mvt);

    glm_vec3_muladds(camera.view_direction,
                     cooked_mvt[1],
                     camera.position);
    glm_vec3_muladds(facing_direction_right,
                     cooked_mvt[0],
                     camera.position);

    // Update camera position with input.
    camera.position[1] +=
        input_state.le_move_world_y_axis.val * capture_fly.speed * delta_time;
}

void BT::Camera::update_frontend_follow_orbit()
{
    // @TODO
    assert(false);
}

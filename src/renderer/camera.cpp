#include "camera.h"

#include "../game_object/game_object.h"
#include "../input_handler/input_handler.h"
#include "cglm/cglm.h"
#include "cglm/euler.h"
#include "cglm/mat3.h"
#include "cglm/mat4.h"
#include "cglm/util.h"
#include "cglm/vec3.h"
#include "imgui.h"
#include "logger.h"
#include <array>
#include <cmath>
#include <memory>
#include <string>

using std::array;
using std::make_unique;
using std::string;


namespace BT
{

struct Camera::Data
{
    bool is_hovering_over_game_viewport{ false };
    function<void(bool)> cursor_lock_fn;

    struct Camera_3D
    {
        float_t fov;
        float_t aspect_ratio;
        float_t z_near;
        float_t z_far;
        vec3 position;
        vec3 view_direction;
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

        bool prev_f1_pressed{ false };
        bool prev_rclick_cam{ false };

        struct Capture_fly
        {
            float_t sensitivity{ 0.1f };
            float_t speed{ 20.0f };
        } capture_fly;

        bool request_follow_orbit{ false };

        struct Follow_orbit
        {
            // Settings.
            float_t sensitivity_x{ 0.01875f * 0.25f * 0.25f };
            float_t sensitivity_y{ 0.0125f * 0.25f * 0.25f };
            float_t orbit_x_auto_turn_disable_time{ 0.5f };
            float_t orbit_x_auto_turn_speed{ 1.0f };
            float_t orbit_x_auto_turn_max_influence_magnitude{ 5.0f }; // @THOUGHT: Should this be based off the input instead of the effective velocity?
            float_t cam_distance{ 10.0f };
            float_t cam_return_to_distance_speed{ 10.0f };
            float_t follow_offset_y{ 1.0f };

            // Internal state.
            Game_object_pool* game_object_pool{ nullptr };
            UUID game_object_ref{ UUID() };
            vec3 current_follow_pos{ 0.0f, 3.0f, 0.0f };
            vec2 orbits{ glm_rad(0.0f), glm_rad(30.0f) };
            float_t max_orbit_y{ glm_rad(89.0f) };
            float_t auto_turn_disable_timer{ 0.0f };
            float_t current_cam_distance;
        } follow_orbit;
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
    glm_vec3_copy(vec3{ 27.448f, 16.669f, 31.33f }, m_data->camera.position);
    glm_vec3_copy(vec3{ -0.566f, -0.53f, -0.632f }, m_data->camera.view_direction);
}

BT::Camera::~Camera() = default;  // For smart pimpl.

// Callbacks.
void BT::Camera::set_callbacks(function<void(bool)>&& cursor_lock_fn)
{
    m_data->cursor_lock_fn = std::move(cursor_lock_fn);
}

void BT::Camera::set_game_object_pool(Game_object_pool& pool)
{
    m_data->frontend.follow_orbit.game_object_pool = &pool;
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

void BT::Camera::get_view_direction(vec3& out_view_direction)
{
    glm_vec3_copy(m_data->camera.view_direction, out_view_direction);
}

// Camera frontend.
void BT::Camera::set_follow_object(UUID game_object_ref)
{
    m_data->frontend.follow_orbit.game_object_ref = game_object_ref;
}

BT::UUID BT::Camera::get_follow_object()
{
    return m_data->frontend.follow_orbit.game_object_ref;
}

void BT::Camera::request_follow_orbit()
{
    m_data->frontend.request_follow_orbit = true;
}

bool BT::Camera::is_capture_fly()
{
    return (m_data->frontend.state == Data::Frontend::FRONTEND_CAMERA_STATE_CAPTURE_FLY);
}

bool BT::Camera::is_follow_orbit()
{
    return (m_data->frontend.state == Data::Frontend::FRONTEND_CAMERA_STATE_FOLLOW_ORBIT);
}

void BT::Camera::update_frontend(Input_handler::State const& input_state,
                                 float_t delta_time)
{
    auto& frontend{ m_data->frontend };

    // Use input state.
    bool on_press_le_rclick_cam{ !frontend.prev_rclick_cam && input_state.le_rclick_cam.val };
    bool on_release_le_rclick_cam{ frontend.prev_rclick_cam && !input_state.le_rclick_cam.val };
    frontend.prev_rclick_cam = input_state.le_rclick_cam.val;

    bool on_press_le_f1{ !frontend.prev_f1_pressed && input_state.le_f1.val };
    frontend.prev_f1_pressed = input_state.le_f1.val;

    // Process state.
    bool first{ true };
    Data::Frontend::Frontend_state prev_state;
    do
    {
        prev_state = frontend.state;

        switch (frontend.state)
        {
            case Data::Frontend::FRONTEND_CAMERA_STATE_STATIC:
                update_frontend_static(input_state, on_press_le_rclick_cam, on_press_le_f1, first);
                break;

            case Data::Frontend::FRONTEND_CAMERA_STATE_CAPTURE_FLY:
                update_frontend_capture_fly(input_state, delta_time, on_release_le_rclick_cam);
                break;

            case Data::Frontend::FRONTEND_CAMERA_STATE_FOLLOW_ORBIT:
                update_frontend_follow_orbit(input_state, delta_time, on_press_le_f1, first);
                break;

            default:
                // Unsupported state attempted to be handled.
                assert(false);
                break;
        }

        first = false;
    } while (prev_state != frontend.state);
}

bool BT::Camera::is_mouse_captured()
{
    return (m_data->frontend.state ==
                Data::Frontend::FRONTEND_CAMERA_STATE_CAPTURE_FLY);
}

// ImGui.
void BT::Camera::set_hovering_over_game_viewport(bool hovering)
{
    m_data->is_hovering_over_game_viewport = hovering;
}

void BT::Camera::render_imgui(std::function<std::string(char const* const)> const& window_name_w_context_fn)
{
    auto& camera{ m_data->camera };

    ImGui::Begin(window_name_w_context_fn("Camera properties").c_str());
    ImGui::PushItemWidth(ImGui::GetFontSize() * -10);
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
    ImGui::PopItemWidth();
    ImGui::End();
}

// Frontend functions.
void BT::Camera::change_frontend_state(uint32_t to_state)
{
    // Exit state.
    switch (m_data->frontend.state)
    {
        case Data::Frontend::FRONTEND_CAMERA_STATE_CAPTURE_FLY:
            // Release cursor.
            m_data->cursor_lock_fn(false);
            break;

        case Data::Frontend::FRONTEND_CAMERA_STATE_FOLLOW_ORBIT:
            // Release cursor.
            m_data->cursor_lock_fn(false);
            break;

        default: break;
    }

    m_data->frontend.state = static_cast<Data::Frontend::Frontend_state>(to_state);

    // Enter state.
    switch (m_data->frontend.state)
    {
        case Data::Frontend::FRONTEND_CAMERA_STATE_CAPTURE_FLY:
            // Lock cursor.
            m_data->cursor_lock_fn(true);
            break;

        case Data::Frontend::FRONTEND_CAMERA_STATE_FOLLOW_ORBIT:
            // Lock cursor.
            m_data->cursor_lock_fn(true);

            // Init current cam distance.
            m_data->frontend.follow_orbit.current_cam_distance =
                m_data->frontend.follow_orbit.cam_distance;
            break;

        default: break;
    }
}

void BT::Camera::update_frontend_static(Input_handler::State const& input_state,
                                        bool on_press_le_rclick_cam,
                                        bool on_press_le_f1,
                                        bool first)
{
    // Interstate checks.
    if (m_data->frontend.request_follow_orbit || (first && on_press_le_f1))
    {
        change_frontend_state(Data::Frontend::FRONTEND_CAMERA_STATE_FOLLOW_ORBIT);
        m_data->frontend.request_follow_orbit = false;
    }

    if (m_data->is_hovering_over_game_viewport && on_press_le_rclick_cam)
    {
        change_frontend_state(Data::Frontend::FRONTEND_CAMERA_STATE_CAPTURE_FLY);
    }
}

void BT::Camera::update_frontend_capture_fly(Input_handler::State const& input_state,
                                             float_t delta_time,
                                             bool on_release_le_rclick_cam)
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

    // Interstate checks.
    if (m_data->frontend.request_follow_orbit)
    {
        m_data->frontend.request_follow_orbit = false;
    }
    if (on_release_le_rclick_cam)
    {
        change_frontend_state(Data::Frontend::FRONTEND_CAMERA_STATE_STATIC);
    }
}

void BT::Camera::update_frontend_follow_orbit(Input_handler::State const& input_state,
                                              float_t delta_time,
                                              bool on_press_le_f1,
                                              bool first)
{
    auto& camera{ m_data->camera };
    auto& fo{ m_data->frontend.follow_orbit };

    vec3 mvt_velocity{ 0.0f, 0.0f, 0.0f };

    if (!fo.game_object_ref.is_nil())
    {   // Follow game object with camera.
        auto game_obj{ fo.game_object_pool->get_one_no_lock(fo.game_object_ref) };
        assert(game_obj != nullptr);

        // Copy prev follow position.
        vec3 from_follow_pos;
        glm_vec3_copy(fo.current_follow_pos, from_follow_pos);

        // Update follow position.
        game_obj->get_transform_handle().get_position(fo.current_follow_pos);

        // Calc mvt velocity (@NOTE: deltatime independant).
        glm_vec3_sub(fo.current_follow_pos, from_follow_pos, mvt_velocity);
        glm_vec3_scale(mvt_velocity, 1.0f / delta_time, mvt_velocity);
    }

    float_t auto_turn_delta{ 0.0f };
    mvt_velocity[1] = 0.0f;
    if (fo.auto_turn_disable_timer > 0.0f)
    {
        // Work to expire auto turn disable timer.
        fo.auto_turn_disable_timer -= delta_time;
    }
    else if (float_t mvt_velo_flat_mag{ glm_vec3_norm2(mvt_velocity) }; mvt_velo_flat_mag > 1e-6)
    {
        // Calc auto turn influence.
        float_t auto_turn_mag_influence{
            glm_clamp_zo(sqrtf(mvt_velo_flat_mag) / fo.orbit_x_auto_turn_max_influence_magnitude) };

        vec3 mvt_velo_flat_normal;
        glm_vec3_normalize_to(mvt_velocity, mvt_velo_flat_normal);

        vec3 cam_view_flat_normal;
        glm_vec3_copy(camera.view_direction, cam_view_flat_normal);
        cam_view_flat_normal[1] = 0.0f;
        glm_vec3_normalize(cam_view_flat_normal);

        float_t mvt_cam_dot{ glm_vec3_dot(mvt_velo_flat_normal, cam_view_flat_normal) };
        float_t auto_turn_dot_influence{ 1.0f - abs(mvt_cam_dot) };

        // Calc target orbit angle.
        float_t target_orbit_x{
            atan2f(mvt_velo_flat_normal[0], mvt_velo_flat_normal[2])
                + (mvt_cam_dot < 0.0f ? glm_rad(0.0f) : glm_rad(180.0f)) };

        // Calc auto turn value.
        static auto angle_diff_fn = [](float_t current, float_t target) {
            float_t diff{ target - current };
            while (diff < -glm_rad(180.0f))
                diff += glm_rad(360.0f);
            while (diff > glm_rad(180.0f))
                diff -= glm_rad(360.0f);
            return diff;
        };
        auto_turn_delta = fo.orbit_x_auto_turn_speed
                              * glm_signf(angle_diff_fn(fo.orbits[0], target_orbit_x))
                              * auto_turn_mag_influence
                              * auto_turn_dot_influence
                              * delta_time;
        // @DEBUG: @NOCHECKIN: @TEMP
        auto_turn_delta = 0.0f;
    }

    if (abs(input_state.look_delta.x.val) > 1e-6f)
    {
        // Disable orbiting for some time for look delta to override.
        auto_turn_delta = 0.0f;
        fo.auto_turn_disable_timer = fo.orbit_x_auto_turn_disable_time;
    }

    // Set new orbit values.
    float_t look_delta_x{ input_state.look_delta.x.val * fo.sensitivity_x + auto_turn_delta };
    float_t look_delta_y{ input_state.look_delta.y.val * fo.sensitivity_y };

    fo.orbits[0] -= look_delta_x;
    while (fo.orbits[0] >= glm_rad(360.0f))
        fo.orbits[0] -= glm_rad(360.0f);
    while (fo.orbits[0] < 0.0f)
        fo.orbits[0] += glm_rad(360.0f);

    fo.orbits[1] += look_delta_y;
    fo.orbits[1] = glm_clamp(fo.orbits[1], -fo.max_orbit_y, fo.max_orbit_y);

    // Calculate look offset.
    vec3 offset_from_follow_obj{ 0.0f, 0.0f, -fo.current_cam_distance };
    mat4 look_rotation;
    glm_euler_zyx(vec3{ fo.orbits[1], fo.orbits[0], 0.0f }, look_rotation);
    glm_mat4_mulv3(look_rotation, offset_from_follow_obj, 0.0f, offset_from_follow_obj);

    // Position camera transform.
    glm_vec3_add(fo.current_follow_pos, vec3{ 0.0f, fo.follow_offset_y, 0.0f}, camera.position);
    glm_vec3_add(camera.position, offset_from_follow_obj, camera.position);

    glm_vec3_negate_to(offset_from_follow_obj, camera.view_direction);
    glm_vec3_normalize(camera.view_direction);

    // Interstate checks.
    if (first && on_press_le_f1)
    {
        change_frontend_state(Data::Frontend::FRONTEND_CAMERA_STATE_STATIC);
    }
}

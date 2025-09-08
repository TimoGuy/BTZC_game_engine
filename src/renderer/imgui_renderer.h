#pragma once

#include <functional>
#include <string>

using std::function;
using std::string;


namespace BT
{

class Camera;
class Game_object;
class Game_object_pool;
class Renderer;
class Input_handler;

class ImGui_renderer
{
public:
    void set_game_obj_pool_ref(Game_object_pool* pool) { m_game_obj_pool = pool; }
    void set_camera_ref(Camera* camera) { m_camera = camera; }
    void set_renderer_ref(Renderer* renderer) { m_renderer = renderer; }
    void set_input_handler_ref(Input_handler* input_handler) { m_input_handler = input_handler; }

    using Switch_scene_fn = function<void(string const&)>;
    void set_switch_scene_callback(Switch_scene_fn&& request_switch_scene_callback)
    {
        m_request_switch_scene_callback = std::move(request_switch_scene_callback);
    }

    void set_selected_game_obj(Game_object* game_obj);
    void render_imgui(float_t delta_time);

private:
    Game_object_pool* m_game_obj_pool{ nullptr };
    Camera* m_camera{ nullptr };
    Renderer* m_renderer{ nullptr };
    Input_handler* m_input_handler{ nullptr };
    Switch_scene_fn m_request_switch_scene_callback;

    void render_imgui__level_editor_context(bool enter, float_t delta_time);
    void render_imgui__animation_frame_data_editor_context(bool enter, float_t delta_time);
};

}  // namespace BT

#pragma once

#include <functional>

using std::function;


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
    void set_callbacks(function<void()>&& asdfasdf);

    void set_selected_game_obj(Game_object* game_obj);
    void render_imgui();

private:
    Game_object_pool* m_game_obj_pool{ nullptr };
    Camera* m_camera{ nullptr };
    Renderer* m_renderer{ nullptr };
    Input_handler* m_input_handler{ nullptr };

    void render_imgui__level_editor_context();
    void render_imgui__animation_frame_data_editor_context();
};

}  // namespace BT

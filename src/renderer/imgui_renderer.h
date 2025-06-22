#pragma once

#include <functional>

using std::function;


namespace BT
{

class Camera;
class Game_object;
class Game_object_pool;
class Renderer;

class ImGui_renderer
{
public:
    void set_game_obj_pool_ref(Game_object_pool* pool) { m_game_obj_pool = pool; }
    void set_camera_ref(Camera* camera) { m_camera = camera; }
    void set_renderer_ref(Renderer* renderer) { m_renderer = renderer; }
    void set_callbacks(function<void()>&& asdfasdf);

    void set_selected_game_obj(Game_object* game_obj);
    void render_imgui();

private:
    Game_object_pool* m_game_obj_pool{ nullptr };
    Camera* m_camera{ nullptr };
    Renderer* m_renderer{ nullptr };
};

}  // namespace BT

#pragma once

#include <string>

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
    ImGui_renderer();

    void set_camera_ref(Camera* camera) { m_camera = camera; }
    void set_renderer_ref(Renderer* renderer) { m_renderer = renderer; }
    void set_input_handler_ref(Input_handler* input_handler) { m_input_handler = input_handler; }

    /// This flag is retrieved for getting different behavior amongst systems when the AFA editor is
    /// being used.
    /// As in, this is @HACK ... very verrry hacky!!!  -Thea 2025/11/09
    bool is_anim_frame_data_editor_context() const;

    void render_imgui(float_t delta_time);

private:
    Game_object_pool* m_game_obj_pool{ nullptr };
    Camera* m_camera{ nullptr };
    Renderer* m_renderer{ nullptr };
    Input_handler* m_input_handler{ nullptr };

    void render_imgui__level_editor_context(bool enter, float_t delta_time);
    void render_imgui__animation_frame_data_editor_context(bool enter, float_t delta_time);
};

}  // namespace BT

#pragma once

#include "entt/entity/fwd.hpp"

#include <string>
#include <unordered_map>
#include <vector>


namespace BT
{
namespace world
{

class Scene_loader
{
public:
    Scene_loader();

    /// Loads a new scene additively.
    void load_scene(std::string const& scene_name);

    /// Unloads all scenes that are loaded using this scene loader.
    void unload_all_scenes();

    /// Process all requests for scene loading.
    void process_scene_loading_requests(entt::registry& reg);

    /// Data type for containing loaded scenes.
    using Scene_entity_list_t = std::vector<entt::entity>;

private:
    std::unordered_map<std::string, Scene_entity_list_t> m_loaded_scenes;

    std::vector<std::string> m_load_scene_requests;
    bool m_unload_all_scenes_request{ false };
};

}  // namespace world
}  // namespace BT

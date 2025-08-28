#pragma once

#include "nlohmann/json.hpp"
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;


namespace BT
{

class Model;

namespace anim_frame_action
{

enum Serialization_mode
{
    SERIAL_MODE_SERIALIZE = 0,
    SERIAL_MODE_DESERIALIZE
};

struct Runtime_data  // @TODO: Perhaps enforcement of using the correct model for an animator using this runtime state?  -Thea 2025/08/26
{
    Runtime_data() = default;
    Runtime_data(std::string const& fname);

    Model const* model{ nullptr };

    struct Control_item
    {
        std::string name;
        // @TODO: @HERE: A type of control, i.e. override var region, event, etc.
    };
    std::vector<Control_item> control_items;

    struct Animation_frame_action_timeline
    {
        struct Region
        {
            uint32_t ctrl_item_idx;  // Idx in `control_items`.
            int32_t  start_frame;
            int32_t  end_frame;
        };
        std::vector<Region> regions;
    };
    std::vector<Animation_frame_action_timeline> anim_frame_action_timelines;  // Same order as `model_animations`.

    void serialize(Serialization_mode mode,
                   json& node_ref);
};

// @COPYPASTA: See `mesh.h`
class Bank
{
public:
    static void emplace(std::string const& name, Runtime_data&& runtime_state);
    static Runtime_data const& get(std::string const& name);
    static std::vector<std::string> get_all_names();

private:
    inline static std::unordered_map<std::string, Runtime_data> s_runtime_states;
};

}  // namespace anim_frame_action
}  // namespace BT

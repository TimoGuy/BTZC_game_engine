#include "render_object.h"

#include "../game_object/game_object.h"
#include "../service_finder/service_finder.h"
#include "animator_template.h"
#include "btglm.h"
#include "btlogger.h"
#include "mesh.h"


BT::Render_object::Render_object(Game_object& game_obj,
                                 Render_layer layer,
                                 Renderable_ifc const* renderable /*= nullptr*/)
    : m_game_obj(game_obj)
    , m_layer(layer)
    , m_renderable(renderable)
{
    // Check that the layer is a single layer, not an aggregate layer.
    constexpr uint32_t k_num_shifts{ sizeof(Render_layer) * 8 };
    uint32_t num_matches{ 0 };
    for (uint32_t i = 0; i < k_num_shifts; i++)
    {
        static_assert(sizeof(Render_layer) <= sizeof(uint64_t));
        uint64_t check_val{ 1ui64 << i };
        if (static_cast<uint64_t>(layer) & check_val)
        {
            num_matches++;
        }
    }

    if (num_matches == 0)
    {
        logger::printe(logger::ERROR, "No render layer assigned to render object.");
        assert(false);
    }
    else if (num_matches > 1)
    {
        logger::printe(logger::ERROR, "Multiple render layers assigned to render object.");
        assert(false);
    }
}

void BT::Render_object::render(Render_layer active_layers,
                               Material_ifc* override_material /*= nullptr*/)
{
    if (m_layer & active_layers)
    {
        mat4 transform;
#if BTZC_REFACTOR_TO_ENTT
        assert(false);  // @TODO implement.
#else
        m_game_obj.get_transform_handle().get_transform_as_mat4(transform);
#endif  // !BTZC_REFACTOR_TO_ENTT
        m_renderable->render(transform, override_material);
    }
}

#if !BTZC_REFACTOR_TO_ENTT
// Scene_serialization_ifc.
void BT::Render_object::scene_serialize(Scene_serialization_mode mode, json& node_ref)
{
    if (mode == SCENE_SERIAL_MODE_SERIALIZE)
    {
        node_ref["guid"] = UUID_helper::to_pretty_repr(get_uuid());

        node_ref["renderable"]["type"] = m_renderable->get_type_str();
        node_ref["renderable"]["model_name"] = m_renderable->get_model_name();

        if (m_renderable->get_type_str() == "Deformed_model")
        {
            node_ref["renderable"]["animator_template"] = (m_renderable->get_model_name() + ".btanitor");
            node_ref["renderable"]["anim_frame_action_ctrls"] = (m_renderable->get_model_name() + ".btafa");
        }

        node_ref["render_layer"] = static_cast<uint8_t>(m_layer);
    }
    else if (mode == SCENE_SERIAL_MODE_DESERIALIZE)
    {
        assign_uuid(node_ref["guid"], true);

        std::string rend_type{ node_ref["renderable"]["type"] };
        if (rend_type == "Model")
        {
            m_renderable = Model_bank::get_model(node_ref["renderable"]["model_name"]);
        }
        else if (rend_type == "Deformed_model")
        {
            auto const& model{ *Model_bank::get_model(node_ref["renderable"]["model_name"]) };
            set_deformed_model(std::make_unique<Deformed_model>(model));

            auto animator{ std::make_unique<Model_animator>(model) };

            service_finder::find_service<Animator_template_bank>()
                .load_animator_template_into_animator(*animator,
                                                      node_ref["renderable"]["animator_template"]);

            animator->configure_anim_frame_action_controls(
                &anim_frame_action::Bank::get(node_ref["renderable"]["anim_frame_action_ctrls"]));

            set_model_animator(std::move(animator));
        }
        else
        {   // Unsupported renderable type.
            assert(false);
            return;
        }

        m_layer = Render_layer(static_cast<uint8_t>(node_ref["render_layer"]));
    }
}
#endif  // !BTZC_REFACTOR_TO_ENTT

BT::UUID BT::Render_object_pool::emplace(Render_object&& rend_obj)
{
    UUID uuid{ rend_obj.get_uuid() };
    if (uuid.is_nil())
    {
        logger::printe(logger::ERROR, "Invalid UUID passed in.");
        assert(false);
    }

    wait_until_free_then_block();
    m_render_objects.emplace(uuid, std::move(rend_obj));
    unblock();

    return uuid;
}

void BT::Render_object_pool::remove(UUID key)
{
    wait_until_free_then_block();
    if (m_render_objects.find(key) == m_render_objects.end())
    {
        // Fail bc key was invalid.
        logger::printef(logger::ERROR, "Render object key %llu does not exist", key);
        assert(false);
        return;
    }

    m_render_objects.erase(key);
    unblock();
}

vector<BT::Render_object*> BT::Render_object_pool::checkout_all_render_objs()
{
    wait_until_free_then_block();

    vector<Render_object*> all_rend_objs;
    all_rend_objs.reserve(m_render_objects.size());

    for (auto it = m_render_objects.begin(); it != m_render_objects.end(); it++)
    {
        all_rend_objs.emplace_back(&it->second);
    }

    return all_rend_objs;
}

vector<BT::Render_object*> BT::Render_object_pool::checkout_render_obj_by_key(vector<UUID>&& keys)
{
    wait_until_free_then_block();

    vector<Render_object*> some_rend_objs;
    some_rend_objs.reserve(keys.size());

    for (auto key : keys)
    {
        if (key.is_nil())
        {
            logger::printe(logger::WARN, "Render object UUID is invalid");
            assert(false);
        }
        else if (m_render_objects.find(key) == m_render_objects.end())
        {
            logger::printef(logger::WARN, "Render object UUID %s does not exist", UUID_helper::to_pretty_repr(key).c_str());
            assert(false);
        }
        else
        {
            // Checkout render object.
            some_rend_objs.emplace_back(&m_render_objects.at(key));
        }
    }

    return some_rend_objs;
}

void BT::Render_object_pool::return_render_objs(vector<Render_object*>&& render_objs)
{
    // Assumed that this is the end of using the renderobject list.
    // @TODO: Prevent misuse of this function.
    // (@IDEA: Make it so that instead of a vector use a class that has a release function or a dtor)
    // @COPYPASTA: See `game_object.cpp`
    (void)render_objs;
    unblock();
}

// Synchronization. (@COPYPASTA: See `game_object.cpp`)
void BT::Render_object_pool::wait_until_free_then_block()
{
    while (true)
    {
        bool unblocked{ false };
        if (m_blocked.compare_exchange_weak(unblocked, true))
        {   // Exchange succeeded and is now in blocking state.
            break;
        }
    }
}

void BT::Render_object_pool::unblock()
{
    m_blocked.store(false);
}

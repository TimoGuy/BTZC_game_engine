#include "render_object.h"

#include "cglm/affine.h"
#include "cglm/cglm.h"
#include "logger.h"
#include "mesh.h"


BT::Render_object::Render_object(Model const& model,
                                 Render_layer layer,
                                 mat4 init_transform,
                                 physics_object_key_t tethered_phys_obj /*= (physics_object_key_t)-1*/)
    : m_model(model)
    , m_layer(layer)
    , m_tethered_phys_obj(tethered_phys_obj)
{
    glm_mat4_copy(init_transform, m_transform);

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

void BT::Render_object::render(Render_layer active_layers)
{
    if (m_layer & active_layers)
    {
        m_model.render_model(m_transform);
    }
}

void BT::Render_object::set_transform(mat4 transform)
{
    glm_mat4_copy(transform, m_transform);
}

void BT::Render_object::get_position(vec3& position)
{
    vec4 pos4;
    glm_vec4_copy(m_transform[3], pos4);  // Copied from `glm_decompose()`.
    glm_vec3(pos4, position);
}

BT::render_object_key_t BT::Render_object_pool::emplace(Render_object&& rend_obj)
{
    render_object_key_t key{ m_next_key++ };

    wait_until_free_then_block();
    m_render_objects.emplace(key, std::move(rend_obj));
    unblock();

    return key;
}

void BT::Render_object_pool::remove(render_object_key_t key)
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

vector<BT::Render_object*> BT::Render_object_pool::checkout_render_obj_by_key(vector<render_object_key_t>&& keys)
{
    wait_until_free_then_block();

    vector<Render_object*> some_rend_objs;
    some_rend_objs.reserve(keys.size());

    for (auto key : keys)
    {
        if (m_render_objects.find(key) == m_render_objects.end())
        {
            // Fail bc key was invalid.
            logger::printef(logger::WARN, "Render object key %llu does not exist", key);
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

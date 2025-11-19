#pragma once

#include "../physics_engine/physics_engine.h"
#include "../uuid/uuid_ifc.h"
#include "render_layer.h"
#include "btglm.h"
#include "material.h"
#include "mesh.h"
#include "model_animator.h"
#include <atomic>
#include <memory>
#include <string>
#include <vector>

using std::atomic_bool;
using std::atomic_uint64_t;
using std::string;
using std::unique_ptr;
using std::vector;


namespace BT
{

class Game_object;
class Physics_object;

class Render_object : public UUID_ifc
{
public:
    Render_object(Render_layer layer);

    Renderable_ifc const* get_renderable() { return m_renderable; }

    void set_model(Model const* model)
    {
        assert(model != nullptr);
        m_renderable = model;

        // Remove any traces of deformed model if included.
        if (m_deformed_model != nullptr)
            m_deformed_model.reset();
        if (m_model_animator != nullptr)
            m_model_animator.reset();
    }

    void set_deformed_model(unique_ptr<Deformed_model>&& deformed_model)
    {
        m_deformed_model = std::move(deformed_model);
        if (m_deformed_model != nullptr)
            m_renderable = m_deformed_model.get();
    }
    Deformed_model* get_deformed_model() { return m_deformed_model.get(); }

    void set_model_animator(unique_ptr<Model_animator>&& model_animator)
    {
        m_model_animator = std::move(model_animator);
    }
    Model_animator* get_model_animator() { return m_model_animator.get(); }

    /// Read and write handle for render transform.
    vec4* render_transform() { return m_render_transform; }

    void render(Render_layer active_layers,
                Material_ifc* override_material = nullptr);

private:
    Render_layer m_layer;
    Renderable_ifc const* m_renderable;
    unique_ptr<Deformed_model> m_deformed_model{ nullptr };  // For owning a deformed model (since models are stored in a bank).
    unique_ptr<Model_animator> m_model_animator{ nullptr };

    mat4 m_render_transform = GLM_MAT4_IDENTITY_INIT;
};

// @COPYPASTA: Not quite copypasta. It's a little bit different.
class Render_object_pool
{
public:
    UUID emplace(Render_object&& rend_obj);
    void remove(UUID key);
    vector<Render_object*> checkout_all_render_objs();
    vector<Render_object*> checkout_render_obj_by_key(vector<UUID>&& keys);
    void return_render_objs(vector<Render_object*>&& render_objs);

    /// Gets number of render objects in this pool.
    size_t get_num_render_objects() const;

private:
    unordered_map<UUID, Render_object> m_render_objects;

    // Synchronization.
    atomic_bool m_blocked{ false };

    void wait_until_free_then_block();
    void unblock();
};

}  // namespace BT

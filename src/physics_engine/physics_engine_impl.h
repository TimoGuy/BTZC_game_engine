#pragma once

#include "Jolt/Jolt.h"  // @NOTE: Must appear first.
#include "Jolt/Core/JobSystemSingleThreaded.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "physics_engine.h"
#include "physics_engine_impl_custom_listeners.h"
#include "physics_engine_impl_layers.h"
#include "physics_engine_impl_obj_vs_broad_phase_filter.h"
#include <memory>

using std::unique_ptr;


namespace BT
{

class Physics_engine::Phys_impl
{
public:
    Phys_impl();
    ~Phys_impl();

    JPH::PhysicsSystem* get_physics_system_ptr();
    JPH::TempAllocator* get_physics_temp_allocator_ptr();

    void update(float_t physics_delta_time);

private:
    unique_ptr<JPH::Factory> m_factory;
    unique_ptr<JPH::TempAllocatorImpl> m_jolt_temp_allocator;
    unique_ptr<JPH::JobSystemSingleThreaded> m_job_system;

    BP_layer_interface_impl m_broad_phase_layer_ifc_impl;
    Object_vs_broad_phase_layer_filter_impl m_obj_vs_broad_phase_layer_filter;
    Object_layer_pair_filter_impl m_obj_layer_pair_filter;

    My_body_activation_listener m_body_activation_listener;
    My_contact_listener m_contact_listener;
    
    std::unique_ptr<JPH::PhysicsSystem> m_physics_system;
};

}  // namespace BT

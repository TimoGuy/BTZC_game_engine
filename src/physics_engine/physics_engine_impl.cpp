#include "physics_engine_impl.h"

#include "Jolt/Jolt.h"  // @NOTE: Must appear first.
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/IssueReporting.h"
#include "Jolt/Core/Memory.h"
#include "Jolt/Core/JobSystemSingleThreaded.h"  // @FUTURE: Replace this with custom job system version in the future.
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/RegisterTypes.h"
#include "logger.h"
#include "physics_engine.h"
#include "physics_engine_impl_error_callbacks.h"
#include <memory>

using std::make_unique;


BT::Physics_engine::Phys_impl::Phys_impl()
{
    // Setup physics.
    JPH::RegisterDefaultAllocator();

    JPH::Trace = phys_engine::trace_impl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = phys_engine::assert_failed_impl);

    m_factory = make_unique<JPH::Factory>();
    JPH::Factory::sInstance = m_factory.get();
    JPH::RegisterTypes();

    m_jolt_temp_allocator = make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    constexpr uint32_t k_max_physics_jobs{ 2048 };
    constexpr uint32_t k_max_physics_barriers{ 8 };

    // @NOTE: The point at which Jolt physics' multithreaded performance starts
    //   to degrade (w/ current version).  -Thea 2025/03/13
    constexpr int32_t k_max_concurrency{ 16 };

    m_job_system = make_unique<JPH::JobSystemSingleThreaded>(k_max_physics_jobs);

    // Setup physics world.
    m_physics_system = std::make_unique<JPH::PhysicsSystem>();

    constexpr uint32_t k_max_bodies{ 65536 };
    constexpr uint32_t k_num_body_mutexes{ 0 };  // Default settings is no mutexes to protect bodies from concurrent access.
    constexpr uint32_t k_max_body_pairs{ 65536 };
    constexpr uint32_t k_max_contact_constraints{ 10240 };
    m_physics_system->Init(k_max_bodies,
                           k_num_body_mutexes,
                           k_max_body_pairs,
                           k_max_contact_constraints,
                           m_broad_phase_layer_ifc_impl,
                           m_obj_vs_broad_phase_layer_filter,
                           m_obj_layer_pair_filter);

    m_physics_system->SetBodyActivationListener(&m_body_activation_listener);
    m_physics_system->SetContactListener(&m_contact_listener);

    // @HARDCODE: From previous project.  -Thea 2025/03/13 (2023/09/29)
    m_physics_system->SetGravity(JPH::Vec3{ 0.0f, -37.5f, 0.0f });

    m_physics_system->OptimizeBroadPhase();
}

BT::Physics_engine::Phys_impl::~Phys_impl()
{
    JPH::UnregisterTypes();
}

JPH::PhysicsSystem* BT::Physics_engine::Phys_impl::get_physics_system_ptr()
{
    return m_physics_system.get();
}

JPH::TempAllocator* BT::Physics_engine::Phys_impl::get_physics_temp_allocator_ptr()
{
    return m_jolt_temp_allocator.get();
}

void BT::Physics_engine::Phys_impl::update(float_t physics_delta_time)
{
    JPH::EPhysicsUpdateError error =
        m_physics_system->Update(physics_delta_time,
                                 1,
                                 m_jolt_temp_allocator.get(),
                                 m_job_system.get());

    if (error != JPH::EPhysicsUpdateError::None)
    {
        logger::printe(logger::ERROR, "Error occurred during physics update.");
        assert(false);
    }
}

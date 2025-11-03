////////////////////////////////////////////////////////////////////////////////////////////////////
/// @copyright (c) 2025 Thea Bennett
/// @brief Properties of a simulating world.
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "world_properties.h"

#include "service_finder/service_finder.h"


BT::world::World_properties_container::World_properties_container()
{
    BT_SERVICE_FINDER_ADD_SERVICE(World_properties_container, this);
}

BT::world::World_properties& BT::world::World_properties_container::get_data_handle()
{
    // @TODO: make this thread safe (I think in the future access will be multithreaded).
    return m_data;
}

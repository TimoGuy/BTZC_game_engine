////////////////////////////////////////////////////////////////////////////////////////////////////
/// @copyright (c) 2025 Thea Bennett
/// @brief Properties of a simulating world.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once


namespace BT
{
namespace world
{

struct World_properties
{
    /// Whether simulation is on or not.
    bool is_simulation_running;


    friend class World_properties_container;
private:
    World_properties() = default;
};

class World_properties_container
{
public:
    World_properties_container();

    World_properties& get_data_handle();

private:
    World_properties m_data;
};


}  // namespace world
}  // namespace BT

#include "player_character_movement.h"

#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/player_character.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"

#include <cassert>


void BT::system::player_character_movement()
{
    auto view{
        service_finder::find_service<Entity_container>()
            .get_ecs_registry()
            .view<component::Player_character, component::Created_physics_object_reference>()
    };

    // @TEMP: @UNSURE: Only support one player character and fail if not the first.
    bool is_first{ true };

    for (auto entity : view)
    {
        assert(is_first);

        // Get input for player character.
        assert(false);  // @TODO: Implement!

        // Transform directional input to camera space.

        // Process input into character movement logic.

        // Apply movement logic outputs to physics object character controller inputs.

        // End of first iteration.
        is_first = false;
    }
}
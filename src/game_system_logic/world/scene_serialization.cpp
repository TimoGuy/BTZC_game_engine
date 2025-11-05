#include "scene_serialization.h"

#include "btjson.h"
#include "btzc_game_engine.h"
#include "game_system_logic/component/component_registry.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"


BT::world::Scene_serialization BT::world::deserialize_scene_data_from_disk(
    std::string const& scene_name)
{
    return Scene_serialization(json_load_from_disk(BTZC_GAME_ENGINE_ASSET_SCENE_PATH + scene_name));
}

void BT::world::serialize_scene_data_to_disk(Scene_serialization const& scene_data,
                                             std::string const& scene_name)
{
    json_save_to_disk(json(scene_data), BTZC_GAME_ENGINE_ASSET_SCENE_PATH + scene_name);
}

BT::world::Scene_serialization::Entity_serialization BT::world::serialize_entity(UUID entity_uuid)
{
    // Reference: https://github.com/skypjack/entt/issues/88#issuecomment-1276858022

    auto& ent_cont{ service_finder::find_service<Entity_container>() };
    auto& reg{ ent_cont.get_ecs_registry() };
    auto ecs_ent_id{ ent_cont.find_entity(entity_uuid) };

    Scene_serialization::Entity_serialization new_ent_serialized{ .entity_uuid = entity_uuid };

    for (auto&& curr : reg.storage())
        if (curr.second.contains(ecs_ent_id))
        {   // Found a component.
            entt::id_type comp_id{ curr.first };

            // Make sure that this component is serializable.
            auto poss_comp_members{ component::serialize_component_of_entity(ecs_ent_id, comp_id) };
            if (!poss_comp_members.has_value())
                continue;

            // Serialize component.
            new_ent_serialized.components.emplace_back(
                Scene_serialization::Entity_serialization::Component_serialization{
                    .type_name = component::find_component_name(comp_id),
                    .members_j = poss_comp_members.value(),
                });
        }

    return new_ent_serialized;
}

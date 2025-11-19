#pragma once

#include "btjson.h"
#include "renderer/render_layer.h"
#include "uuid/uuid.h"



namespace BT
{
namespace component
{

/// Serializable set of info that shows what kind of render object to create in the renderer.
struct Render_object_settings
{
    Render_layer render_layer{ RENDER_LAYER_DEFAULT };
    std::string model_name{ "" };

    bool is_deformed{ false };
    std::string animator_template_name{ "" };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Render_object_settings,
        render_layer,
        model_name,
        is_deformed,
        animator_template_name
    );
};

/// Holder of a reference to a created render obj. The absence of this component w/ the presence of
/// `Render_object_settings` means that a render object needs to be created for this entity.
struct Created_render_object_reference
{
    UUID render_obj_uuid_ref;
};

}  // namespace component
}  // namespace BT

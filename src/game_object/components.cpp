#include "components.h"

#include "../renderer/animator_template.h"
#include "../renderer/model_animator.h"
#include "../renderer/mesh.h"
#include "../service_finder/service_finder.h"


BT::component_system::Component_model_animator::~Component_model_animator()
{
    if (product != nullptr)
        delete product;
}

BT::Model_animator& BT::component_system::Component_model_animator::get_product()
{
    if (product == nullptr)
    {   // Build animator.
        product = new Model_animator(*Model_bank::get_model(animatable_model_name));

        service_finder::find_service<Animator_template_bank>().load_animator_template_into_animator(
            *product,
            animator_template_name);

        product->configure_anim_frame_action_controls(
            &anim_frame_action::Bank::get(anim_frame_action_ctrls));
    }

    return *product;
}

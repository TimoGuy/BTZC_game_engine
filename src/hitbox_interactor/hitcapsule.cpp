#include "hitcapsule.h"

#include "../service_finder/service_finder.h"


// @TODO: @HERE: Hitcapsule.
// @TODO: @HERE: Hitcapsule_group.

// Hitcapsule_group_overlap_solver.
BT::Hitcapsule_group_overlap_solver::Hitcapsule_group_overlap_solver()
{
    // Add self as service.
    BT_SERVICE_FINDER_ADD_SERVICE(Hitcapsule_group_overlap_solver, this);
}

BT::UUID BT::Hitcapsule_group_overlap_solver::add_group(Hitcapsule_group const& group)
{}

void BT::Hitcapsule_group_overlap_solver::remove_group(UUID group_id)
{}

void BT::Hitcapsule_group_overlap_solver::update_overlaps()
{}

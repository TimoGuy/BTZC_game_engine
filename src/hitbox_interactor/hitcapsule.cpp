#include "hitcapsule.h"

#include "../renderer/debug_render_job.h"
#include "../renderer/model_animator.h"
#include "../service_finder/service_finder.h"
#include "btglm.h"
#include "btlogger.h"
#include "cglm/util.h"
#include "cglm/vec3.h"

#include <cassert>


// Hitcapsule.
void BT::Hitcapsule::init_calc_info(Model_animator const& animator)
{
    auto& joint_name_to_idx{ animator.get_model_skin().joint_name_to_idx };

    if (!connecting_bone_name.empty())
        calcd_bone_mat_idx = joint_name_to_idx.at(connecting_bone_name);
    if (!connecting_bone_name_2.empty())
        calcd_bone_mat_idx_2 = joint_name_to_idx.at(connecting_bone_name_2);

    glm_vec3_copy(origin_a.raw, calcd_origin_a);
    glm_vec3_copy(origin_b.raw, calcd_origin_b);

    calc_orig_pt_distance();
}

void BT::Hitcapsule::update_transform(mat4 base_transform, std::vector<mat4s> const& joint_matrices)
{
    static auto s_eval_combined_transforms_fn = [](size_t connecting_bone_mat_idx,
                                                   size_t connecting_bone_mat_idx_2,
                                                   mat4 base_transform,
                                                   std::vector<mat4s> const& joint_matrices,
                                                   mat4& out_combined_trans,
                                                   mat4& out_combined_trans_2) {
        if (connecting_bone_mat_idx == (size_t)-1)
        {
            glm_mat4_copy(base_transform, out_combined_trans);
            glm_mat4_copy(out_combined_trans, out_combined_trans_2);
        }
        else
        {
            glm_mat4_mul(base_transform,
                         const_cast<vec4*>(joint_matrices[connecting_bone_mat_idx].raw),
                         out_combined_trans);

            // Second connecting bone.
            if (connecting_bone_mat_idx_2 == (size_t)-1)
            {
                glm_mat4_copy(out_combined_trans, out_combined_trans_2);
            }
            else
            {
                glm_mat4_mul(base_transform,
                             const_cast<vec4*>(joint_matrices[connecting_bone_mat_idx_2].raw),
                             out_combined_trans_2);
            }
        }
    };

    // Update capsule origins with combined transform(s).
    mat4 combined_trans;
    mat4 combined_trans_2;
    s_eval_combined_transforms_fn(calcd_bone_mat_idx,
                                  calcd_bone_mat_idx_2,
                                  base_transform,
                                  joint_matrices,
                                  combined_trans,
                                  combined_trans_2);

    glm_mat4_mulv3(combined_trans, origin_a.raw, 1.0f, calcd_origin_a);
    glm_mat4_mulv3(combined_trans_2, origin_b.raw, 1.0f, calcd_origin_b);

    // Recalc origin point dependent vars.
    calc_orig_pt_distance();
}

void BT::Hitcapsule::calc_orig_pt_distance()
{
    calcd_orig_pts_dist = glm_vec3_distance(calcd_origin_a, calcd_origin_b);
}

void BT::Hitcapsule::emplace_debug_render_repr(vec4 color) const
{
    get_main_debug_line_pool().emplace_debug_line_based_capsule(
        const_cast<float_t*>(calcd_origin_a),
        const_cast<float_t*>(calcd_origin_b),
        radius,
        color,
#define BT_HITCAPSULE_DEBUG_RENDER_REPRESENTATION_LONG_HOLD 0
#if BT_HITCAPSULE_DEBUG_RENDER_REPRESENTATION_LONG_HOLD
        0.5f);
#else
        0.03f);
#endif
}


// Hitcapsule_group.
void BT::Hitcapsule_group::set_enabled(bool enabled)
{
    m_enabled = enabled;
}

bool BT::Hitcapsule_group::is_enabled() const
{
    return m_enabled;
}

BT::Hitcapsule_group::Type BT::Hitcapsule_group::get_type() const
{
    return m_type;
}

std::vector<BT::Hitcapsule>& BT::Hitcapsule_group::get_capsules()
{
    return m_capsules;
}

void BT::Hitcapsule_group::emplace_debug_render_repr() const
{   // Generate color.
    vec4s color;
    switch (m_type)
    {
        case HITBOX_TYPE_RECEIVE_HURT:
            color = (m_enabled
                     ? vec4s{ 0.509, 0.465, 0.990 }
                     : vec4s{ 0.201, 0.183, 0.390 });
            break;

        case HITBOX_TYPE_GIVE_HURT:
            color = (m_enabled
                     ? vec4s{ 0.950, 0.427, 0.00 }
                     : vec4s{ 0.530, 0.288, 0.0901 });
            break;
    }

    // "Draw" hitcapsules.
    for (auto& hitcapsule : m_capsules)
    {
        hitcapsule.emplace_debug_render_repr(color.raw);
    }
}


// Hitcapsule_group_set.
BT::Hitcapsule_group_set::~Hitcapsule_group_set()
{
    unregister_from_overlap_solver();
}

void BT::Hitcapsule_group_set::replace_and_reregister(Hitcapsule_group_set const& other)
{
    auto& overlap_solver{ service_finder::find_service<Hitcapsule_group_overlap_solver>() };

    if (m_is_registered_in_overlap_solver)
    {   // Unregister self if currently registered.  @COPYPASTA with `unregister_from_overlap_solver()`
        m_is_registered_in_overlap_solver =
            !overlap_solver.remove_group_set(*this);
        assert(!m_is_registered_in_overlap_solver);
    }

    *this = other;

    m_is_registered_in_overlap_solver = overlap_solver.add_group_set(*this);
    assert(m_is_registered_in_overlap_solver);
}

void BT::Hitcapsule_group_set::unregister_from_overlap_solver()
{
    if (m_is_registered_in_overlap_solver)
    {
        // Unregister to cleanup.
        m_is_registered_in_overlap_solver =
            !service_finder::find_service<Hitcapsule_group_overlap_solver>().remove_group_set(*this);
    }
}

void BT::Hitcapsule_group_set::connect_animator(Model_animator const& animator)
{
    for (auto& group : m_hitcapsule_grps)
    for (auto& capsule : group.get_capsules())
    {
        capsule.init_calc_info(animator);
    }

    m_is_connected_to_animator = true;
}

std::vector<BT::Hitcapsule_group>& BT::Hitcapsule_group_set::get_hitcapsule_groups()
{
    return m_hitcapsule_grps;
}

void BT::Hitcapsule_group_set::emplace_debug_render_repr() const
{
    for (auto& group : m_hitcapsule_grps)
    {
        group.emplace_debug_render_repr();
    }
}

std::vector<BT::Hitcapsule const*> BT::Hitcapsule_group_set::fetch_all_enabled_hitcapsules(
    Hitcapsule_group::Type hitcapsule_type) const
{
    std::vector<Hitcapsule const*> result_hitcapsules;

    for (auto& group : m_hitcapsule_grps)
        if (group.is_enabled() && group.get_type() == hitcapsule_type)
        {
            auto const& hitcapsules{ const_cast<Hitcapsule_group&>(group).get_capsules() };  // @HACK.

            for (auto& hitcapsule : hitcapsules)
                result_hitcapsules.emplace_back(&hitcapsule);
        }

    result_hitcapsules.shrink_to_fit();
    return result_hitcapsules;    
}


// Hitcapsule_group_overlap_solver.
BT::Hitcapsule_group_overlap_solver::Hitcapsule_group_overlap_solver()
{
    // Add self as service.
    BT_SERVICE_FINDER_ADD_SERVICE(Hitcapsule_group_overlap_solver, this);
}

namespace
{
constexpr char const* const k_add_log_str =
    "(+) Added hitcapsule group set to overlap solver:     ";
constexpr char const* const k_remove_log_str =
    "(-) Removed hitcapsule group set from overlap solver: ";
}  // namespace

bool BT::Hitcapsule_group_overlap_solver::add_group_set(Hitcapsule_group_set& group_set)
{
    bool success{ false };
    if (m_group_sets.emplace(&group_set).second)
    {
        logger::printef(logger::TRACE,
                        "%s%p",
                        k_add_log_str,
                        (void*)&group_set);
        success = true;
    }
    else
        logger::printe(logger::ERROR, "Emplacing hitcapsule group set failed.");

    assert(success);
    return success;
}

bool BT::Hitcapsule_group_overlap_solver::remove_group_set(Hitcapsule_group_set& group_set)
{
    bool success{ false };
    auto iter{ m_group_sets.find(&group_set) };
    if (iter != m_group_sets.end())
    {
        m_group_sets.erase(iter);
        logger::printef(logger::TRACE,
                        "%s%p",
                        k_remove_log_str,
                        (void*)&group_set);
        success = true;
    }

    return success;
}

size_t BT::Hitcapsule_group_overlap_solver::get_num_group_sets() const
{
    return m_group_sets.size();
}

void BT::Hitcapsule_group_overlap_solver::update_overlaps()
{
    for (auto grp_set_ptr_a : m_group_sets)
    for (auto grp_set_ptr_b : m_group_sets)
    if (grp_set_ptr_a != grp_set_ptr_b)
    {   // Collect capsules for check.
        auto give_hurt_caps_a{ grp_set_ptr_a->fetch_all_enabled_hitcapsules(
            Hitcapsule_group::HITBOX_TYPE_GIVE_HURT) };
        auto rece_hurt_caps_b{ grp_set_ptr_b->fetch_all_enabled_hitcapsules(
            Hitcapsule_group::HITBOX_TYPE_RECEIVE_HURT) };

        // Overlap check (A hurts B).
        // Does broad phase first, then if passes, does narrow phase.
        // @NOTE: Only A hurts B is necessary, since the n^2 loops. The reverse case will eventually
        //        be covered with the loops.
        bool found_overlap{ false };
        {
            for (size_t a_i = 0; a_i < give_hurt_caps_a.size() && !found_overlap; a_i++)
            for (size_t b_i = 0; b_i < rece_hurt_caps_b.size() && !found_overlap; b_i++)
            {
                if (check_broad_phase_hitcapsule_pair(*give_hurt_caps_a[a_i], *rece_hurt_caps_b[b_i]) &&
                    check_narrow_phase_hitcapsule_pair(*give_hurt_caps_a[a_i], *rece_hurt_caps_b[b_i]))
                {   // Found overlap! No more need to look.
                    found_overlap = true;
                    break;
                }
            }
        }

        if (found_overlap)
        {   // Report that A hurt B!
        #if 0
            assert(false);  // @TODO: Implement!
        #endif  // 0
        }
    }

    // Submit debug drawing for all hitcapsules.
    for (auto group_set_ptr : m_group_sets)
    {
        group_set_ptr->emplace_debug_render_repr();
    }
}

bool BT::Hitcapsule_group_overlap_solver::check_broad_phase_hitcapsule_pair(
    Hitcapsule const& give_hurt_capsule,
    Hitcapsule const& receive_hurt_capsule) const
{   // Sphere-to-sphere overlap.
    auto cap_a_origin_a{ const_cast<float_t*>(give_hurt_capsule.calcd_origin_a) };
    auto cap_a_origin_b{ const_cast<float_t*>(give_hurt_capsule.calcd_origin_b) };
    auto cap_a_orig_pts_dist{ give_hurt_capsule.calcd_orig_pts_dist };
    auto cap_a_radius{ give_hurt_capsule.radius };

    auto cap_b_origin_a{ const_cast<float_t*>(receive_hurt_capsule.calcd_origin_a) };
    auto cap_b_origin_b{ const_cast<float_t*>(receive_hurt_capsule.calcd_origin_b) };
    auto cap_b_orig_pts_dist{ give_hurt_capsule.calcd_orig_pts_dist };
    auto cap_b_radius{ receive_hurt_capsule.radius };

    // Calculate bounding sphere over capsules.
    vec3 a_mdpt_origin = GLM_VEC3_ZERO_INIT;
    glm_vec3_muladds(cap_a_origin_a, 0.5f, a_mdpt_origin);
    glm_vec3_muladds(cap_a_origin_b, 0.5f, a_mdpt_origin);
    float_t a_bp_sphere_rad{ cap_a_orig_pts_dist * 0.5f + cap_a_radius };

    vec3 b_mdpt_origin = GLM_VEC3_ZERO_INIT;
    glm_vec3_muladds(cap_b_origin_a, 0.5f, b_mdpt_origin);
    glm_vec3_muladds(cap_b_origin_b, 0.5f, b_mdpt_origin);
    float_t b_bp_sphere_rad{ cap_b_orig_pts_dist * 0.5f + cap_b_radius };

    // Check for overlap.
    bool found_overlap{ false };

    float_t combined_rads{ a_bp_sphere_rad + b_bp_sphere_rad };
    if (glm_vec3_distance2(a_mdpt_origin, b_mdpt_origin) < combined_rads * combined_rads)
    {   // Found overlap!
        found_overlap = true;

        #if 0
        // BT_TRACEF("Broad phase passed.\n"
        //           "  A orig A: (%.3f, %.3f, %.3f)\n"
        //           "  A orig B: (%.3f, %.3f, %.3f)\n"
        //           "  A c_dist:  %.3f\n"
        //           "  A radius: %.3f\n"
        //           "  B orig A: (%.3f, %.3f, %.3f)\n"
        //           "  B orig B: (%.3f, %.3f, %.3f)\n"
        //           "  B c_dist:  %.3f\n"
        //           "  B radius: %.3f\n",
        //           cap_a_origin_a[0], cap_a_origin_a[1], cap_a_origin_a[2],
        //           cap_a_origin_b[0], cap_a_origin_b[1], cap_a_origin_b[2],
        //           cap_a_orig_pts_dist,
        //           cap_a_radius,
        //           cap_b_origin_a[0], cap_b_origin_a[1], cap_b_origin_a[2],
        //           cap_b_origin_b[0], cap_b_origin_b[1], cap_b_origin_b[2],
        //           cap_b_orig_pts_dist,
        //           cap_b_radius);

        get_main_debug_line_pool().emplace_debug_line_based_capsule(
            a_mdpt_origin,
            a_mdpt_origin,
            a_bp_sphere_rad,
            vec4{ 0.0285, 0.570, 0.00 },
        #if BT_HITCAPSULE_DEBUG_RENDER_REPRESENTATION_LONG_HOLD
            0.5f);
        #else
            0.03f);
        #endif

        get_main_debug_line_pool().emplace_debug_line_based_capsule(
            b_mdpt_origin,
            b_mdpt_origin,
            b_bp_sphere_rad,
            vec4{ 0.478, 0.590, 0.0295 },
        #if BT_HITCAPSULE_DEBUG_RENDER_REPRESENTATION_LONG_HOLD
            0.5f);
        #else
            0.03f);
        #endif

        #endif  // 0
    }

    return found_overlap;
}

bool BT::Hitcapsule_group_overlap_solver::check_narrow_phase_hitcapsule_pair(
    Hitcapsule const& give_hurt_capsule,
    Hitcapsule const& receive_hurt_capsule) const
{   // Capsule-to-capsule overlap.

    #define PYTHON_CODE 0
    #if PYTHON_CODE
    import numpy as np

    def closestDistanceBetweenLines(a0,a1,b0,b1,clampAll=False,clampA0=False,clampA1=False,clampB0=False,clampB1=False):

        // ''' Given two lines defined by numpy.array pairs (a0,a1,b0,b1)
        //     Return the closest points on each segment and their distance
        // '''

        // If clampAll=True, set all clamps to True
        if clampAll:
            clampA0=True
            clampA1=True
            clampB0=True
            clampB1=True


        // Calculate denomitator
        A = a1 - a0
        B = b1 - b0
        magA = np.linalg.norm(A)
        magB = np.linalg.norm(B)
        
        _A = A / magA
        _B = B / magB
        
        cross = np.cross(_A, _B);
        denom = np.linalg.norm(cross)**2
        
        
        // If lines are parallel (denom=0) test if lines overlap.
        // If they don't overlap then there is a closest point solution.
        // If they do overlap, there are infinite closest positions, but there is a closest distance
        if not denom:
            d0 = np.dot(_A,(b0-a0))
            
            // Overlap only possible with clamping
            if clampA0 or clampA1 or clampB0 or clampB1:
                d1 = np.dot(_A,(b1-a0))
                
                // Is segment B before A?
                if d0 <= 0 >= d1:
                    if clampA0 and clampB1:
                        if np.absolute(d0) < np.absolute(d1):
                            return a0,b0,np.linalg.norm(a0-b0)
                        return a0,b1,np.linalg.norm(a0-b1)
                    
                    
                // Is segment B after A?
                elif d0 >= magA <= d1:
                    if clampA1 and clampB0:
                        if np.absolute(d0) < np.absolute(d1):
                            return a1,b0,np.linalg.norm(a1-b0)
                        return a1,b1,np.linalg.norm(a1-b1)
                    
                    
            // Segments overlap, return distance between parallel segments
            return None,None,np.linalg.norm(((d0*_A)+a0)-b0)
            
        
        
        // Lines criss-cross: Calculate the projected closest points
        t = (b0 - a0);
        detA = np.linalg.det([t, _B, cross])
        detB = np.linalg.det([t, _A, cross])

        t0 = detA/denom;
        t1 = detB/denom;

        pA = a0 + (_A * t0) // Projected closest point on segment A
        pB = b0 + (_B * t1) // Projected closest point on segment B


        // Clamp projections
        if clampA0 or clampA1 or clampB0 or clampB1:
            if clampA0 and t0 < 0:
                pA = a0
            elif clampA1 and t0 > magA:
                pA = a1
            
            if clampB0 and t1 < 0:
                pB = b0
            elif clampB1 and t1 > magB:
                pB = b1
                
            // Clamp projection A
            if (clampA0 and t0 < 0) or (clampA1 and t0 > magA):
                dot = np.dot(_B,(pA-b0))
                if clampB0 and dot < 0:
                    dot = 0
                elif clampB1 and dot > magB:
                    dot = magB
                pB = b0 + (_B * dot)
        
            // Clamp projection B
            if (clampB0 and t1 < 0) or (clampB1 and t1 > magB):
                dot = np.dot(_A,(pB-a0))
                if clampA0 and dot < 0:
                    dot = 0
                elif clampA1 and dot > magA:
                    dot = magA
                pA = a0 + (_A * dot)

        
        return pA,pB,np.linalg.norm(pA-pB)
    #endif  // PYTHON_CODE


    // @TODO: Try the new method above!
    // Ref: https://stackoverflow.com/questions/2824478/shortest-distance-between-two-line-segments





    #define DRAGONITESPAM_METHOD 0
    #if DRAGONITESPAM_METHOD
    #define GML_CODE 0
    #if GML_CODE
    // Ref: https://github.com/DragoniteSpam-GameMaker-Tutorials/3DCollisions/blob/7cd8c3a7006db84094816b750698c039548dc1fc/scripts/Col_Shape_Line/Col_Shape_Line.gml#L170-L203
    function ColLine(start, finish) constructor {
        self.start = start;                     // Vec3
        self.finish = finish;                   // Vec3
        
        var sx = start.x, sy = start.y, sz = start.z;
        var fx = finish.x, fy = finish.y, fz = finish.z;
        self.property_min = new Vector3(min(sx, fx), min(sy, fy), min(sz, fz));
        self.property_max = new Vector3(max(sx, fx), max(sy, fy), max(sz, fz));
        self.property_ray = new ColRay(start, new Vector3(fx - sx, fy - sy, fz - sz));
        self.property_length = point_distance_3d(sx, sy, sz, fx, fy, fz);
        self.property_center = new Vector3(mean(sx, fx), mean(sy, fy), mean(sz, fz));
    };
    
    static NearestConnectionToRay = function(ray) {
        var line1 = self;
        var line2 = ray;
        
        var start = line1.start;
        var finish = line1.finish;
        var origin = line2.origin;
        var dir = line2.direction;
        
        var d1x = finish.x - start.x;
        var d1y = finish.y - start.y;
        var d1z = finish.z - start.z;
        var d2x = dir.x;
        var d2y = dir.y;
        var d2z = dir.z;
        var rx = start.x - origin.x;
        var ry = start.y - origin.y;
        var rz = start.z - origin.z;
        
        var f = dot_product_3d(d2x, d2y, d2z, rx, ry, rz);
        var c = dot_product_3d(d1x, d1y, d1z, rx, ry, rz);
        var b = dot_product_3d(d1x, d1y, d1z, d2y, d2z, d2z);
        var length_squared = dot_product_3d(d1x, d1y, d1z, d1x, d1y, d1z);
        
        // special case if the line segment is actually just
        // two of the same points
        if (length_squared == 0) {
            return new ColLine(start, line2.NearestPoint(start));
        }
        
        var f1 = 0;
        var f2 = 0;
        var denominator = length_squared - b * b;
        
        // if the two lines are parallel, there are infinitely many shortest
        // connecting lines, so you can just pick a random point on line1 to
        // work from - we'll pick the starting point
        if (denominator == 0) {
            f1 = 0;
        } else {
            f1 = clamp((b * f - c - 1) / denominator, 0, 1);
        }
        f2 = f1 * b + f;
        
        if (f2 < 0) {
            f2 = 0;
            f1 = clamp(-c / length_squared, 0, 1);
        }
        
        return new ColLine(
            new Vector3(
                start.x + d1x * f1,
                start.y + d1y * f1,
                start.z + d1z * f1
            ), new Vector3(
                origin.x + d2x * f2,
                origin.y + d2y * f2,
                origin.z + d2z * f2
            )
        );
    };

    static NearestConnectionToLine = function(line) {
        var nearest_connection_to_ray = self.NearestConnectionToRay(line.property_ray);
        
        var start = self.start;
        var finish = self.finish;
        var lvx = finish.x - start.x;
        var lvy = finish.y - start.y;
        var lvz = finish.z - start.z;
        var ldd = dot_product_3d(lvx, lvy, lvz, lvx, lvy, lvz);
        
        var p = nearest_connection_to_ray.start;
        var px = p.x - start.x;
        var py = p.y - start.y;
        var pz = p.z - start.z;
        var t = clamp(dot_product_3d(px, py, pz, lvx, lvy, lvz) / ldd, 0, 1);
        
        var starting_point = new Vector3(
            start.x + lvx * t,
            start.y + lvy * t,
            start.z + lvz * t
        );
        
        var lstart = line.start;
        p = nearest_connection_to_ray.finish;
        px = p.x - lstart.x;
        py = p.y - lstart.y;
        pz = p.z - lstart.z;
        t = clamp(dot_product_3d(px, py, pz, lvx, lvy, lvz) / ldd, 0, 1);
        
        var ending_point = new Vector3(
            lstart.x + lvx * t,
            lstart.y + lvy * t,
            lstart.z + lvz * t
        );
        
        return new ColLine(starting_point, ending_point);
    };
    #endif  // GML_CODE

    // Preparation.
    struct Line_segment
    {
        vec3 pt1;
        vec3 pt2;
    };
    Line_segment cap_a_ls;
    glm_vec3_copy(const_cast<float_t*>(give_hurt_capsule.calcd_origin_a), cap_a_ls.pt1);
    glm_vec3_copy(const_cast<float_t*>(give_hurt_capsule.calcd_origin_b), cap_a_ls.pt2);

    Line_segment cap_b_ls;
    glm_vec3_copy(const_cast<float_t*>(receive_hurt_capsule.calcd_origin_a), cap_b_ls.pt1);
    glm_vec3_copy(const_cast<float_t*>(receive_hurt_capsule.calcd_origin_b), cap_b_ls.pt2);

    // Nearest connection line to rays.
    Line_segment rays_conn_line;
    {
        struct Ray
        {
            vec3 origin;
            vec3 dir;
        };
        Ray ray_a;
        glm_vec3_copy(cap_a_ls.pt1, ray_a.origin);
        glm_vec3_sub(cap_a_ls.pt2, cap_a_ls.pt1, ray_a.dir);

        Ray ray_b;
        glm_vec3_copy(cap_b_ls.pt1, ray_b.origin);
        glm_vec3_sub(cap_b_ls.pt2, cap_b_ls.pt1, ray_b.dir);

        vec3 delta_ray_origin;
        glm_vec3_sub(ray_a.origin, ray_b.origin, delta_ray_origin);

        float_t f{ glm_vec3_dot(ray_b.dir, delta_ray_origin) };
        float_t c{ glm_vec3_dot(ray_a.dir, delta_ray_origin) };
        float_t b{ glm_vec3_dot(ray_a.dir, ray_b.dir) };
        float_t length_sqr{ glm_vec3_dot(ray_a.dir, ray_a.dir) };

        // @TODO: Special case if the line segment is actually just two of the same points.

        float_t f1{ 0 };
        float_t f2{ 0 };
        float_t denominator{ length_sqr - b * b };

        if (glm_eq(denominator, 0))
        {   // Choose starting point since lines are parallel.
            f1 = 0;
        }
        else
        {
            f1 = glm_clamp_zo((b * f - c - 1) / denominator);
        }

        f2 = f1 * b + f;
        if (f2 < 0)
        {
            f2 = 0;
            f1 = glm_clamp_zo(-c / length_sqr);
        }

        // Return connection line.
        glm_vec3_copy(ray_a.origin, rays_conn_line.pt1);
        glm_vec3_muladds(ray_a.dir, f1, rays_conn_line.pt1);

        glm_vec3_copy(ray_b.origin, rays_conn_line.pt2);
        glm_vec3_muladds(ray_b.dir, f2, rays_conn_line.pt2);
    }

    // Nearest connection line to line segments.
    Line_segment lines_conn_line;
    {
        vec3 lv;
        glm_vec3_sub(cap_a_ls.pt2, cap_a_ls.pt1, lv);
        auto ldd{ glm_vec3_dot(lv, lv) };

        vec3 pda;
        glm_vec3_sub(rays_conn_line.pt1, cap_a_ls.pt1, pda);
        auto ta{ glm_clamp_zo(glm_vec3_dot(pda, lv) / ldd) };

        vec3 pdb;
        glm_vec3_sub(rays_conn_line.pt2, cap_b_ls.pt1, pdb);
        auto tb{ glm_clamp_zo(glm_vec3_dot(pdb, lv) / ldd) };

        // Return connection line.
        glm_vec3_copy(cap_a_ls.pt1, lines_conn_line.pt1);
        glm_vec3_muladds(lv, ta, lines_conn_line.pt1);

        glm_vec3_copy(cap_b_ls.pt1, lines_conn_line.pt2);
        glm_vec3_muladds(lv, tb, lines_conn_line.pt2);
    }

    // Copy result to vars for overlap check.
    vec3 best_a;
    glm_vec3_copy(lines_conn_line.pt1, best_a);
    vec3 best_b;
    glm_vec3_copy(lines_conn_line.pt2, best_b);
    auto cap_a_radius{ give_hurt_capsule.radius };
    auto cap_b_radius{ receive_hurt_capsule.radius };
    #endif  // DRAGONITESPAM_METHOD

    #define WICKED_METHOD 0
    #if WICKED_METHOD
    // Ref: https://wickedengine.net/2020/04/capsule-collision-detection/
    auto cap_a_origin_a{ const_cast<float_t*>(give_hurt_capsule.calcd_origin_a) };
    auto cap_a_origin_b{ const_cast<float_t*>(give_hurt_capsule.calcd_origin_b) };
    auto cap_a_radius{ give_hurt_capsule.radius };

    auto cap_b_origin_a{ const_cast<float_t*>(receive_hurt_capsule.calcd_origin_a) };
    auto cap_b_origin_b{ const_cast<float_t*>(receive_hurt_capsule.calcd_origin_b) };
    auto cap_b_radius{ receive_hurt_capsule.radius };

    // Distances betwixt capsule origins.
    auto d0{ glm_vec3_distance2(cap_a_origin_a, cap_b_origin_a) };
    auto d1{ glm_vec3_distance2(cap_a_origin_a, cap_b_origin_b) };
    auto d2{ glm_vec3_distance2(cap_a_origin_b, cap_b_origin_a) };
    auto d3{ glm_vec3_distance2(cap_a_origin_b, cap_b_origin_b) };

    // Find initial best `a` test point.
    vec3 best_a;
    if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1)
        glm_vec3_copy(cap_a_origin_b, best_a);
    else
        glm_vec3_copy(cap_a_origin_a, best_a);

    // Finds point `out_closest_pt` on line segment `a` `b` that is closest to point `pt`.
    static auto const closest_point_on_line_segment_fn =
        [](vec3 a, vec3 b, vec3 pt, vec3& out_closest_pt) {
            // Ref: https://wickedengine.net/2020/04/capsule-collision-detection/
            vec3 ab;
            glm_vec3_sub(b, a, ab);

            vec3 a_to_pt;
            glm_vec3_sub(pt, a, a_to_pt);

            float_t t{ glm_dot(a_to_pt, ab) / glm_dot(ab, ab) };

            // Calc result of lerp.
            glm_vec3_scale(ab, glm_clamp_zo(t), out_closest_pt);
            glm_vec3_add(a, out_closest_pt, out_closest_pt);
        };

    // Find best `b` test point.
    vec3 best_b;
    closest_point_on_line_segment_fn(cap_b_origin_a, cap_b_origin_b, best_a, best_b);

    // Find corrected best `a` test point.
    closest_point_on_line_segment_fn(cap_a_origin_a, cap_a_origin_b, best_b, best_a);

    // Idk one more iter?
    closest_point_on_line_segment_fn(cap_b_origin_a, cap_b_origin_b, best_a, best_b);
    #endif  // WICKED_METHOD




    // Check for overlap.
    bool found_overlap{ false };

    float_t combined_rads{ cap_a_radius + cap_b_radius };
    if (glm_vec3_distance2(best_a, best_b) < combined_rads * combined_rads)
    {   // Found overlap!
        found_overlap = true;
    }

    #if 1
    // @DEBUG.
    get_main_debug_line_pool().emplace_debug_line_based_capsule(
        best_a,
        best_a,
        cap_a_radius,
        found_overlap ? vec4{ 0.610, 0.00, 0.254 } : vec4{ 0.0285, 0.570, 0.00 },
    #if BT_HITCAPSULE_DEBUG_RENDER_REPRESENTATION_LONG_HOLD
        0.5f);
    #else
        0.03f);
    #endif

    get_main_debug_line_pool().emplace_debug_line_based_capsule(
        best_b,
        best_b,
        cap_b_radius,
        found_overlap ? vec4{ 0.560, 0.00560, 0.514 } : vec4{ 0.478, 0.590, 0.0295 },
    #if BT_HITCAPSULE_DEBUG_RENDER_REPRESENTATION_LONG_HOLD
        0.5f);
    #else
        0.03f);
    #endif

    #endif  // @DEBUG

    return found_overlap;
}

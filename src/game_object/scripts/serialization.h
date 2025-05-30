#pragma once

#include "cglm/cglm.h"
#include "logger.h"
#include <cassert>
#include <vector>

using std::vector;


namespace BT
{
namespace Serial
{

inline void push_u64(vector<uint64_t>& in_out_datas, uint64_t new_data)
{
    in_out_datas.emplace_back(new_data);
}

inline uint64_t pop_u64(vector<uint64_t> const& datas, size_t& in_out_read_data_idx)
{
    return datas[in_out_read_data_idx++];
}

inline void push_u64_persistent_state(vector<uint64_t>& in_out_datas)
{
    push_u64(in_out_datas, 0);
}

inline uint64_t& pop_u64_persistent_state(vector<uint64_t> const& datas, size_t& in_out_read_data_idx)
{
    // I like the const nature, but this is an exception. Martyr me.  -Thea 2025/05/30
    auto& datas_unconstified{ const_cast<vector<uint64_t>&>(datas) };
    return datas_unconstified[in_out_read_data_idx++];
}

inline void push_u32(vector<uint64_t>& in_out_datas, uint32_t new_data)
{
    push_u64(in_out_datas, static_cast<uint64_t>(new_data));
}

inline uint32_t pop_u32(vector<uint64_t> const& datas, size_t& in_out_read_data_idx)
{
    return static_cast<uint32_t>(pop_u64(datas, in_out_read_data_idx));
}

inline void push_void_ptr(vector<uint64_t>& in_out_datas, void* new_data)
{
    push_u64(in_out_datas, reinterpret_cast<uintptr_t>(new_data));
}

inline void* pop_void_ptr(vector<uint64_t> const& datas, size_t& in_out_read_data_idx)
{
    return reinterpret_cast<void*>(pop_u64(datas, in_out_read_data_idx));
}

inline void push_vec3(vector<uint64_t>& in_out_datas, vec3 new_data)
{
    static_assert(sizeof(uint64_t) * 2 >= sizeof(float_t) * 3);
    uint64_t parts[2]{ 0, 0 };
    float_t* parts_as_floats{ reinterpret_cast<float_t*>(parts) };
    glm_vec3_copy(new_data, parts_as_floats);

    push_u64(in_out_datas, parts[0]);
    push_u64(in_out_datas, parts[1]);
}

inline void pop_vec3(vector<uint64_t> const& datas, size_t& in_out_read_data_idx, vec3& out_vec3)
{
    static_assert(sizeof(uint64_t) * 2 >= sizeof(float_t) * 3);
    uint64_t parts[2]{
        pop_u64(datas, in_out_read_data_idx),
        pop_u64(datas, in_out_read_data_idx) };

    float_t* parts_as_floats{ reinterpret_cast<float_t*>(parts) };
    glm_vec3_copy(parts_as_floats, out_vec3);
}

}  // namespace Serial

}  // namespace BT

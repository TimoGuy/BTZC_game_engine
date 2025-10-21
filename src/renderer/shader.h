#pragma once

#include "btglm.h"
#include <string>
#include <memory>
#include <utility>
#include <vector>

using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;


namespace BT
{

class Shader
{
public:
    Shader(string const& vert_fname, string const& frag_fname);
    Shader(string const& comp_fname);
    ~Shader();

    void bind() const;
    static void unbind();

    void set_int(string const& param_name, int32_t value) const;
    void set_uint(string const& param_name, uint32_t value) const;
    void set_float(string const& param_name, float_t value) const;
    void set_vec3(string const& param_name, vec3 value) const;
    void set_mat4(string const& param_name, mat4 value) const;
    void bind_texture(string const& param_name, int32_t texture_idx, uint32_t texture_buffer) const;

private:
    uint32_t m_shader_program;

    string read_shader_file(string const& fname);
};

// @COPYPASTA. See "mesh.h", "material.h"
// @TODO: @REFACTOR: This has been copied >2 times, so refactor this.
class Shader_bank
{
public:
    static void emplace_shader(string const& name, unique_ptr<Shader>&& shader);
    static Shader const* get_shader(string const& name);

private:
    inline static vector<pair<string, unique_ptr<Shader>>> s_shaders;
};

}  // namespace BT

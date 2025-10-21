#include "shader.h"

#include "glad/glad.h"
#include "btlogger.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

using std::ifstream;
using std::stringstream;


BT::Shader::Shader(string const& vert_fname, string const& frag_fname)
{
    string vertex_code{ read_shader_file(vert_fname) };
    string fragment_code{ read_shader_file(frag_fname) };

    uint32_t vert_shader;
    uint32_t frag_shader;
    int32_t success;
    char info_log[512];

    // Vertex shader.
    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    auto vertex_code_cstr{ vertex_code.c_str() };
    glShaderSource(vert_shader, 1, &vertex_code_cstr, nullptr);
    glCompileShader(vert_shader);

    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vert_shader, 512, nullptr, info_log);
        logger::printef(logger::ERROR, "Vertex shader: Compilation failed: %s", info_log);
    }

    // Fragment shader.
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    auto fragment_code_cstr{ fragment_code.c_str() };
    glShaderSource(frag_shader, 1, &fragment_code_cstr, nullptr);
    glCompileShader(frag_shader);

    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(frag_shader, 512, nullptr, info_log);
        logger::printef(logger::ERROR, "Fragment shader: Compilation failed: %s", info_log);
    }

    // Link into shader program.
    m_shader_program = glCreateProgram();
    glAttachShader(m_shader_program, vert_shader);
    glAttachShader(m_shader_program, frag_shader);
    glLinkProgram(m_shader_program);

    glGetProgramiv(m_shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(m_shader_program, 512, nullptr, info_log);
        logger::printef(logger::ERROR, "Program: Link failed: %s", info_log);
    }

    // Cleanup.
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
}

BT::Shader::Shader(string const& comp_fname)
{
    string compute_code{ read_shader_file(comp_fname) };

    uint32_t comp_shader;
    int32_t success;
    char info_log[512];

    // Fragment shader.
    comp_shader = glCreateShader(GL_COMPUTE_SHADER);
    auto compute_code_cstr{ compute_code.c_str() };
    glShaderSource(comp_shader, 1, &compute_code_cstr, nullptr);
    glCompileShader(comp_shader);

    glGetShaderiv(comp_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(comp_shader, 512, nullptr, info_log);
        logger::printef(logger::ERROR, "Compute shader: Compilation failed: %s", info_log);
    }

    // Link into shader program.
    m_shader_program = glCreateProgram();
    glAttachShader(m_shader_program, comp_shader);
    glLinkProgram(m_shader_program);

    glGetProgramiv(m_shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(m_shader_program, 512, nullptr, info_log);
        logger::printef(logger::ERROR, "Program: Link failed: %s", info_log);
    }

    // Cleanup.
    glDeleteShader(comp_shader);
}

BT::Shader::~Shader()
{
    glDeleteProgram(m_shader_program);
}

void BT::Shader::bind() const
{
    glUseProgram(m_shader_program);
}

void BT::Shader::unbind()
{
    glUseProgram(0);
}

void BT::Shader::set_int(string const& param_name, int32_t value) const
{
    glUniform1i(glGetUniformLocation(m_shader_program, param_name.c_str()), value);
}

void BT::Shader::set_uint(string const& param_name, uint32_t value) const
{
    glUniform1ui(glGetUniformLocation(m_shader_program, param_name.c_str()), value);
}

void BT::Shader::set_float(string const& param_name, float_t value) const
{
    glUniform1f(glGetUniformLocation(m_shader_program, param_name.c_str()), value);
}

void BT::Shader::set_vec3(string const& param_name, vec3 value) const
{
    glUniform3fv(glGetUniformLocation(m_shader_program, param_name.c_str()),
                 1,
                 value);
}

void BT::Shader::set_mat4(string const& param_name, mat4 value) const
{
    glUniformMatrix4fv(glGetUniformLocation(m_shader_program, param_name.c_str()),
                       1,
                       GL_FALSE,
                       &value[0][0]);
}

void BT::Shader::bind_texture(string const& param_name, int32_t texture_idx, uint32_t texture_buffer) const
{
    set_int(param_name, texture_idx);
    glActiveTexture(GL_TEXTURE0 + texture_idx);
    glBindTexture(GL_TEXTURE_2D, texture_buffer);

}

string BT::Shader::read_shader_file(string const& fname)
{
    string code;
    ifstream shader_file;

    try
    {
        shader_file.open(fname);
        if (!shader_file.is_open())
        {
            auto abs_path{ std::filesystem::absolute(fname).string() };
            logger::printef(logger::ERROR, "File could not be open \"%s\"", abs_path.c_str());
            assert(false);
        }

        stringstream shader_stream;
        shader_stream << shader_file.rdbuf();
        shader_file.close();
        code = shader_stream.str();
    }
    catch (ifstream::failure e)
    {
        logger::printef(logger::ERROR, "File read failed \"%s\"", fname.c_str());
        assert(false);
    }

    return code;
}

void BT::Shader_bank::emplace_shader(string const& name, unique_ptr<Shader>&& shader)
{
    if (get_shader(name) != nullptr)
    {
        // Report shader already exists.
        logger::printef(logger::ERROR, "Shader \"%s\" already exists.", name.c_str());
        assert(false);
        return;
    }

    s_shaders.emplace_back(name, std::move(shader));
}

BT::Shader const* BT::Shader_bank::get_shader(string const& name)
{
    Shader* shader_ptr{ nullptr };

    for (auto& shader : s_shaders)
        if (shader.first == name)
        {
            shader_ptr = shader.second.get();
            break;
        }

    return shader_ptr;
}

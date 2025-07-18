#version 460

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_tex_coord;

layout (location = 0) out vec2 out_tex_coord;


void main()
{
    gl_Position = vec4(in_position, 1.0);
    out_tex_coord = in_tex_coord;
}

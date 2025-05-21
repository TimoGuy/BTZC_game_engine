#version 460

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord;

layout (location = 0) out vec3 out_normal;

uniform mat4 camera_projection_view;
uniform mat4 model_transform;


void main()
{
    gl_Position = camera_projection_view * model_transform * vec4(in_position, 1.0);
    out_normal = in_normal;
}

#version 460

layout (location = 0) in vec3 in_normal;
layout (location = 1) in vec2 in_tex_coord;

layout (location = 0) out vec4 out_frag_color;

uniform sampler2D color_image;
uniform vec3 tint;


void main()
{
    const vec3 k_light_direction = normalize(vec3(2.0, 1.5, 1.0));
    float influence = dot(in_normal, k_light_direction) * 0.5 + 0.5;
    out_frag_color = vec4(texture(color_image, in_tex_coord).rgb * tint * influence, 1.0);
}

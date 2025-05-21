#version 460

layout (location = 0) in vec2 in_tex_coord;

layout (location = 0) out vec4 out_frag_color;

uniform sampler2D hdr_buffer;
uniform float exposure;


void main()
{
    const float gamma = 2.2;
    vec3 hdr_color = texture(hdr_buffer, in_tex_coord).rgb;

    // Reinhard tone mapping w/ gamma correction.
    vec3 mapped = hdr_color / (hdr_color + vec3(1.0));
    mapped = pow(mapped, vec3(1.0 / gamma));

    out_frag_color = vec4(mapped, 1.0);
}

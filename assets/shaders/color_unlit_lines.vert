#version 460

layout (location = 0) smooth out vec4 out_color;

struct Line_data {
    vec4 pos1;
    vec4 pos2;
    vec4 col1;
    vec4 col2;
};

layout (binding = 0, std430) readonly buffer Line_ssbo {
    Line_data lines[];
};

uniform mat4 camera_projection_view;
uniform mat4 model_transform;


void main()
{
    Line_data line_data = lines[gl_InstanceID];
    vec3 pos = (gl_VertexID == 0 ? line_data.pos1 : line_data.pos2).xyz;
    vec4 col = (gl_VertexID == 0 ? line_data.col1 : line_data.col2);

    gl_Position = camera_projection_view * model_transform * vec4(pos, 1.0);
    out_color = col;
}

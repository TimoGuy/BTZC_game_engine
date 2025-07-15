#version 460

layout (location = 0) smooth out vec4 out_color;

struct Line_data {
    vec4 pos1;
    vec4 pos2;
    vec4 col1;
    vec4 col2;
};

layout (binding = 3, std430) readonly buffer Line_ssbo {  // Binding 3 to avoid 0-2 vert attribs.  @CHECK: I think this isn't needed?????
    Line_data lines[];
};

uniform mat4 camera_projection_view;


void main()
{
    Line_data line_data = lines[gl_InstanceID];
    vec3 pos = (gl_VertexID == 0 ? line_data.pos1 : line_data.pos2).xyz;
    vec4 col = (gl_VertexID == 0 ? line_data.col1 : line_data.col2);

    gl_Position = camera_projection_view * vec4(pos, 1.0);
    out_color = col;
}

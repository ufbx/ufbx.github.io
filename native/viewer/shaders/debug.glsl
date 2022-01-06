@ctype vec2 um_vec2
@ctype vec3 um_vec3
@ctype vec4 um_vec4
@ctype mat4 um_mat

@vs debug_vertex

layout(location=0) in vec4 a_position;
layout(location=1) in vec4 a_color;

out vec4 v_color;

void main()
{
    gl_Position = a_position;
    v_color = a_color;
}

@end

@fs debug_pixel

in vec4 v_color;

out vec4 o_color;

void main()
{
    o_color = v_color;
}
@end

@program debug debug_vertex debug_pixel

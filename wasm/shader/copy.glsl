@ctype vec2 um_vec2
@ctype vec3 um_vec3
@ctype vec4 um_vec4
@ctype mat4 um_mat

@vs copy_vertex

@glsl_options flip_vert_y

layout(location=0) in vec2 a_uv;
out vec2 v_uv;

void main()
{
    gl_Position = vec4(a_uv.x * 2.0 - 1.0, a_uv.y * 2.0 - 1.0, 0.5, 1.0);
    v_uv = a_uv;
}

@end

@fs copy_pixel

layout(binding=0) uniform sampler2D u_texture;

in vec2 v_uv;
out vec4 o_color;

void main()
{
    o_color = texture(u_texture, v_uv);
}
@end

@program copy copy_vertex copy_pixel

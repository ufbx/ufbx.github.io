@ctype vec2 um_vec2
@ctype vec3 um_vec3
@ctype vec4 um_vec4
@ctype mat4 um_mat

@vs mesh_vertex

layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;

out vec3 v_normal;

uniform ubo_mesh_vertex {
    mat4 geometry_to_world;
    mat4 world_to_clip;
};

void main()
{
    vec3 geo_pos = a_position;
    vec3 geo_normal = a_normal;

    vec3 world_pos = (geometry_to_world * vec4(a_position, 1.0)).xyz;
    vec3 world_normal = (geometry_to_world * vec4(a_normal, 0.0)).xyz;

    vec4 clip_pos = world_to_clip * vec4(world_pos, 1.0);

    gl_Position = clip_pos;
    v_normal = normalize(world_normal);
}

@end

@fs mesh_pixel

in vec3 v_normal;

out vec4 o_color;

void main()
{
    vec3 l = normalize(vec3(1.0));
    vec3 n = normalize(v_normal);
    float x = dot(n, l) * 0.5 + 0.5;
    o_color = vec4(vec3(x), 1.0);
}
@end

@program mesh mesh_vertex mesh_pixel


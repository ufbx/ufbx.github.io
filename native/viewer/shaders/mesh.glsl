@ctype vec2 um_vec2
@ctype vec3 um_vec3
@ctype vec4 um_vec4
@ctype mat4 um_mat

@vs mesh_vertex

layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;
layout(location=2) in vec4 a_info;

out vec3 v_normal;
out vec2 v_barycentric;
out float v_highlight;

uniform ubo_mesh_vertex {
    mat4 geometry_to_world;
    mat4 world_to_clip;
    float highlight;
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
    v_barycentric = vec2(int(a_info.x) == 1 ? 1.0 : 0.0, int(a_info.x) == 2.0 ? 1.0 : 0.0);
    v_highlight = highlight;
}

@end

@fs mesh_pixel

#extension GL_OES_standard_derivatives : enable

in vec3 v_normal;
in vec2 v_barycentric;
in float v_highlight;

out vec4 o_color;

float wireframeWeight(float width)
{
    vec3 bary = vec3(v_barycentric, 1.0 - v_barycentric.x - v_barycentric.y);
    vec3 dbary = fwidth(bary);
    vec3 wire = bary * (1.0 / dbary);
    return 1.0 - clamp(min(min(wire.x, wire.y), wire.z) - (width - 1.0), 0.0, 1.0);
}

void main()
{
    vec2 pixelPos = gl_FragCoord.xy;
    vec3 l = normalize(vec3(1.0, 1.7, 1.4));
    vec3 n = normalize(v_normal);
    float x = dot(n, l) * 0.4 + 0.4;
    vec3 col = vec3(x);
    float wire = wireframeWeight(1.2);
    col = mix(col, vec3(1.0, 0.8, 0.7), v_highlight * mix(wire, 1.0, 0.3));
    o_color = vec4(col, 1.0);
}
@end

@program mesh mesh_vertex mesh_pixel


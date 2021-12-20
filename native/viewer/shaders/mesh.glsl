@ctype vec2 um_vec2
@ctype vec3 um_vec3
@ctype vec4 um_vec4
@ctype mat4 um_mat

@vs mesh_vertex

layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;
layout(location=2) in int a_vertex_index;

out vec3 v_normal;
out vec2 v_barycentric;
out float v_highlight;

uniform ubo_mesh_vertex {
    mat4 u_geometry_to_world;
    mat4 u_world_to_clip;
    float u_highlight;
    float ui_highlight_cluster;
};

uniform sampler2D u_deform_buffer;
uniform sampler2D u_bone_buffer;

vec4 bufferRead(sampler2D tex, int index)
{
    return texelFetch(tex, ivec2(index & 511, index >> 9), 0);
}

mat4 bufferReadMat(sampler2D tex, int index)
{
    vec4 a = bufferRead(tex, index*4+0);
    vec4 b = bufferRead(tex, index*4+1);
    vec4 c = bufferRead(tex, index*4+2);
    vec4 d = bufferRead(tex, index*4+3);
    return mat4(a, b, c, d);
}

void main()
{
    vec3 geo_pos = a_position;
    vec3 geo_normal = a_normal;

    mat4 geometry_to_world = u_geometry_to_world;

    int bary_index = a_vertex_index & 3;
    int vertex_index = a_vertex_index >> 2;

    vec4 deform_info = bufferRead(u_deform_buffer, vertex_index);
    int num_bones = min(int(deform_info.x), 16);
    int bone_base = int(deform_info.y);
    int highlight_cluster = int(ui_highlight_cluster);

    if (num_bones > 0) {
        geometry_to_world = mat4(vec4(0.0), vec4(0.0), vec4(0.0), vec4(0.0));
    }

    float highlight = u_highlight;

    for (int bone_ix = 0; bone_ix < num_bones; bone_ix++) {
        vec4 bone_info = bufferRead(u_deform_buffer, bone_base + bone_ix);

        {
            int cluster_index = int(bone_info.x);
            float weight = bone_info.y;
            if (cluster_index == ui_highlight_cluster) highlight += weight;
            geometry_to_world += bufferReadMat(u_bone_buffer, cluster_index) * weight;
        }

        {
            int cluster_index = int(bone_info.z);
            float weight = bone_info.w;
            if (cluster_index == ui_highlight_cluster) highlight += weight;
            geometry_to_world += bufferReadMat(u_bone_buffer, cluster_index) * weight;
        }
    }

    vec3 world_pos = (geometry_to_world * vec4(a_position, 1.0)).xyz;
    vec3 world_normal = (geometry_to_world * vec4(a_normal, 0.0)).xyz;

    vec4 clip_pos = u_world_to_clip * vec4(world_pos, 1.0);

    gl_Position = clip_pos;
    v_normal = normalize(world_normal);
    v_barycentric = vec2(bary_index == 1 ? 1.0 : 0.0, bary_index == 2.0 ? 1.0 : 0.0);
    v_highlight = highlight;
}

@end

@fs mesh_pixel

#extension GL_OES_standard_derivatives : enable

uniform ubo_mesh_pixel {
    vec3 highlight_color;
};

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
    col = mix(col, highlight_color, v_highlight * mix(wire, 1.0, 0.3));
    o_color = vec4(col, 1.0);
}
@end

@program mesh mesh_vertex mesh_pixel


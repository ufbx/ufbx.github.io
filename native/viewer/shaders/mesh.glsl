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
    float ui_highlight_channel;
    float ui_highlight_shape;
    float ui_g_cluster_begin;
    float ui_g_keyframe_begin;
};

uniform sampler2D u_deform_buffer;
uniform sampler2D u_global_buffer;

vec4 bufferRead(sampler2D tex, int index)
{
    return texelFetch(tex, ivec2(index & 511, index >> 9), 0);
}

mat4 bufferReadMat(sampler2D tex, int index)
{
    vec4 a = bufferRead(tex, index+0);
    vec4 b = bufferRead(tex, index+1);
    vec4 c = bufferRead(tex, index+2);
    vec4 d = bufferRead(tex, index+3);
    return mat4(a, b, c, d);
}

vec4 quatMul(vec4 a, vec4 b)
{
    return vec4(
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z);
}

void main()
{
    vec3 geo_pos = a_position;
    vec3 geo_normal = a_normal;

    mat4 geometry_to_world = u_geometry_to_world;

    int bary_index = a_vertex_index & 3;
    int vertex_index = a_vertex_index >> 2;

    vec4 deform_info = bufferRead(u_deform_buffer, vertex_index);
    float dq_weight = clamp(fract(deform_info.x) * 2.0, 0.0, 1.0);
    int num_bones = min(int(floor(deform_info.x)), 16);
    int bone_base = int(deform_info.y);
    int highlight_cluster = int(ui_highlight_cluster);

    if (num_bones > 0) {
        geometry_to_world = mat4(vec4(0.0), vec4(0.0), vec4(0.0), vec4(0.0));
    }

    float highlight = u_highlight;

    vec4 q0 = vec4(0.0), qe = vec4(0.0), qs = vec4(0.0);

    for (int bone_ix = 0; bone_ix < num_bones; bone_ix++) {
        vec4 bone_info = bufferRead(u_deform_buffer, bone_base + bone_ix);

        {
            int cluster_index = int(bone_info.x);
            float weight = bone_info.y;
            if (cluster_index == int(ui_highlight_cluster)) highlight += weight;
            int index = int(ui_g_cluster_begin) + cluster_index * 7;
            geometry_to_world += bufferReadMat(u_global_buffer, index) * weight;

            if (dq_weight > 0.0) {
                vec4 vq0 = bufferRead(u_global_buffer, index + 4);
                vec4 vqe = bufferRead(u_global_buffer, index + 5);
                vec4 vqs = bufferRead(u_global_buffer, index + 6);
                float vweight = dot(q0, vq0) < 0.0 ? -weight : weight;
                q0 += vq0 * vweight;
                qe += vqe * vweight;
                qs += vqs * weight;
            }
        }

        {
            int cluster_index = int(bone_info.z);
            float weight = bone_info.w;
            if (cluster_index == int(ui_highlight_cluster)) highlight += weight;
            int index = int(ui_g_cluster_begin) + cluster_index * 7;
            geometry_to_world += bufferReadMat(u_global_buffer, index) * weight;

            if (dq_weight > 0.0) {
                vec4 vq0 = bufferRead(u_global_buffer, index + 4);
                vec4 vqe = bufferRead(u_global_buffer, index + 5);
                vec4 vqs = bufferRead(u_global_buffer, index + 6);
                float vweight = dot(q0, vq0) < 0.0 ? -weight : weight;
                q0 += vq0 * vweight;
                qe += vqe * vweight;
                qs += vqs * weight;
            }
        }
    }

    if (dq_weight > 0.0) {
		float rcp_len = 1.0 / length(q0);
		float rcp_len2x2 = 2.0 * rcp_len * rcp_len;
        vec4 rotation = q0 * rcp_len;
        vec3 scale = qs.xyz;
        vec3 translation;
		translation.x = rcp_len2x2 * (- qe.w*q0.x + qe.x*q0.w - qe.y*q0.z + qe.z*q0.y);
		translation.y = rcp_len2x2 * (- qe.w*q0.y + qe.x*q0.z + qe.y*q0.w - qe.z*q0.x);
		translation.z = rcp_len2x2 * (- qe.w*q0.z - qe.x*q0.y + qe.y*q0.x + qe.z*q0.w);

        vec4 q = rotation;
        float sx = 2.0 * scale.x, sy = 2.0 * scale.y, sz = 2.0 * scale.z;
        float xx = q.x*q.x, xy = q.x*q.y, xz = q.x*q.z, xw = q.x*q.w;
        float yy = q.y*q.y, yz = q.y*q.z, yw = q.y*q.w;
        float zz = q.z*q.z, zw = q.z*q.w;
        mat4 dq_matrix = mat4(
            sx * (- yy - zz + 0.5),
            sx * (+ xy + zw),
            sx * (- yw + xz),
            0.0,
            sy * (- zw + xy),
            sy * (- xx - zz + 0.5),
            sy * (+ xw + yz),
            0.0,
            sz * (+ xz + yw),
            sz * (- xw + yz),
            sz * (- xx - yy + 0.5),
            0.0,
            translation.x,
            translation.y,
            translation.z,
            1.0);

        geometry_to_world *= 1.0 - dq_weight;
        geometry_to_world += dq_matrix * dq_weight;
    }

    int num_blends = min(int(deform_info.z), 16);
    int blend_base = int(deform_info.w);

    for (int blend_ix = 0; blend_ix < num_blends; blend_ix++) {
        vec4 blend_info = bufferRead(u_deform_buffer, blend_base + blend_ix);
        int keyframe_index = int(blend_info.x);

        int index = int(ui_g_keyframe_begin) + keyframe_index;
        vec4 keyframe_info = bufferRead(u_global_buffer, index);
        float weight = keyframe_info.x;
        geo_pos += blend_info.yzw * weight;

        if (int(keyframe_info.y) == int(ui_highlight_channel) || int(keyframe_info.z) == int(ui_highlight_shape)) {
            highlight += 1.0;
        }
    }

    vec3 world_pos = (geometry_to_world * vec4(geo_pos, 1.0)).xyz;
    vec3 world_normal = (geometry_to_world * vec4(geo_normal, 0.0)).xyz;

    vec4 clip_pos = u_world_to_clip * vec4(world_pos, 1.0);

    gl_Position = clip_pos;
    v_normal = normalize(world_normal);
    v_barycentric = vec2(bary_index == 1 ? 1.0 : 0.0, bary_index == 2.0 ? 1.0 : 0.0);
    v_highlight = min(highlight, 1.0);
}

@end

@fs mesh_pixel

#extension GL_OES_standard_derivatives : enable

uniform ubo_mesh_pixel {
    vec3 highlight_color;
    float pixel_scale;
};

in vec3 v_normal;
in vec2 v_barycentric;
in float v_highlight;

out vec4 o_color;

float wireframeWeight(float width)
{
    vec3 bary = vec3(v_barycentric, 1.0 - v_barycentric.x - v_barycentric.y);
    vec3 dx = dFdx(bary), dy = dFdy(bary);
    vec3 dbary = sqrt(dx*dx + dy*dy);
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
    float wire = wireframeWeight(1.2 * pixel_scale);
    col = mix(col, highlight_color, v_highlight * mix(wire, 1.0, 0.3));
    o_color = vec4(col, 1.0);
}
@end

@program mesh mesh_vertex mesh_pixel


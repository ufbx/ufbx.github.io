
@vs icon_vertex

layout(location=0) in vec4 a_position;
layout(location=1) in vec2 a_uv;
layout(location=2) in vec4 a_sdf_threshold;
layout(location=3) in vec4 a_color;
layout(location=4) in vec4 a_outline_color;

out vec2 v_uv;
out vec4 v_sdf_threshold;
out vec4 v_color;
out vec4 v_outline_color;

void main()
{
    gl_Position = a_position;
    v_uv = a_uv;
    v_sdf_threshold = a_sdf_threshold;
    v_color = a_color;
    v_outline_color = a_outline_color;
}

@end

@fs icon_pixel

uniform sampler2D icon_atlas;

in vec2 v_uv;
in vec4 v_sdf_threshold;
in vec4 v_color;
in vec4 v_outline_color;

out vec4 o_color;

void main()
{
    float sdf = 1.0 - texture(icon_atlas, v_uv).x;
    vec4 th = v_sdf_threshold;

    vec4 color;
    if (sdf < th.x) {
        color = v_color;
    } else if (sdf < th.y) {
        color = mix(v_color, v_outline_color, (sdf - th.x) / (th.y - th.x));
    } else if (sdf < th.z) {
        color = v_outline_color;
    } else if (sdf < th.w) {
        color = mix(v_outline_color, vec4(v_outline_color.xyz, 0.0), (sdf - th.z) / (th.w - th.z));
    } else {
        color = vec4(0.0);
    }

    o_color = color;
}

@end

@program icon icon_vertex icon_pixel


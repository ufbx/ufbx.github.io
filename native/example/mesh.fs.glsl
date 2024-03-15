#version 330

uniform vec3 base_color;

in vec3 v_normal;
in vec2 v_uv;

out vec4 o_color;

void main() {
    float l = dot(normalize(v_normal), normalize(vec3(1.0))) * 0.5 + 0.5;
    o_color = vec4(base_color * l, 1.0);
}

#version 330

uniform mat4 object_to_world;
uniform mat4 world_to_clip;

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

out vec3 v_normal;
out vec2 v_uv;

void main() {
    vec3 world_position = (object_to_world * vec4(position, 1.0)).xyz;
    gl_Position = world_to_clip * vec4(world_position, 1.0);
    v_normal = (object_to_world * vec4(normal, 0.0)).xyz;
    v_uv = uv;
}

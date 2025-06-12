#version 460 core

layout(location = 0) in vec3 position;

out vec2 texCoord;

void main() {
    texCoord = position.xy * 0.5 + 0.5;
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
#version 460 core

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform float uWaterLevel;
uniform vec3 uWaterColor;

void main() {
    // Store world position and water level flag
    gPosition = vec4(FragPos, 1.0); // w = 1.0 indicates water surface
    
    // Store normal and material properties
    vec3 normal = normalize(Normal);
    gNormal = vec4(normal, 0.5); // w = 0.5 indicates water material
}
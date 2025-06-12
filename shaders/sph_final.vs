#version 460 core

// Simple passthrough vertex shader for fullscreen quad

out vec2 vTexCoord;

// Fullscreen triangle trick
void main() {
    float x = float(((gl_VertexID & 1) << 2) - 1);
    float y = float(((gl_VertexID & 2) << 1) - 1);
    
    gl_Position = vec4(x, y, 0.0, 1.0);
    vTexCoord = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);
}
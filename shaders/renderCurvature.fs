#version 460 core

// Curvature flow for smoothing depth buffer

uniform sampler2D inputDepth;
uniform mat4 projection;
uniform ivec2 screenSize;

const float NEAR = 0.1f;
const float FAR = 100.0f;

in vec2 TexCoord;
out float FragDepth;

// Convert depth from non-linear to linear
float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));
}

void main() {
    vec2 texelSize = 1.0 / vec2(screenSize);
    
    // Sample depth values in 3x3 neighborhood
    float center = texture(inputDepth, TexCoord).r;
    
    // Early out for background
    if (center >= 0.999) {
        FragDepth = center;
        return;
    }
    
    // Get neighboring depths
    float left = texture(inputDepth, TexCoord + vec2(-texelSize.x, 0.0)).r;
    float right = texture(inputDepth, TexCoord + vec2(texelSize.x, 0.0)).r;
    float top = texture(inputDepth, TexCoord + vec2(0.0, texelSize.y)).r;
    float bottom = texture(inputDepth, TexCoord + vec2(0.0, -texelSize.y)).r;
    
    // Compute Laplacian
    float laplacian = left + right + top + bottom - 4.0 * center;
    
    // Apply smoothing with small time step
    float dt = 0.1;
    float newDepth = center + dt * laplacian;
    
    // Clamp to valid range
    FragDepth = clamp(newDepth, 0.0, 1.0);
}
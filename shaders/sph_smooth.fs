#version 460 core

// Curvature flow smoothing shader
// Based on "Screen Space Fluid Rendering with Curvature Flow" by van der Laan et al.

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uDepthTexture;
uniform ivec2 uScreenSize;

void main() {
    vec2 texelSize = 1.0 / vec2(uScreenSize);
    
    // Sample neighboring depths
    float depth_c = texture(uDepthTexture, vTexCoord).r;
    float depth_l = texture(uDepthTexture, vTexCoord + vec2(-texelSize.x, 0.0)).r;
    float depth_r = texture(uDepthTexture, vTexCoord + vec2( texelSize.x, 0.0)).r;
    float depth_b = texture(uDepthTexture, vTexCoord + vec2(0.0, -texelSize.y)).r;
    float depth_t = texture(uDepthTexture, vTexCoord + vec2(0.0,  texelSize.y)).r;
    
    // Skip background pixels
    if (depth_c >= 0.999) {
        fragColor = vec4(depth_c);
        return;
    }
    
    // Simple smoothing filter in depth space
    // This is a simplified version - full curvature flow is more complex
    float smoothed = (depth_c * 4.0 + depth_l + depth_r + depth_b + depth_t) / 8.0;
    
    // Apply smoothing step
    float dt = 0.3; // Smoothing strength
    float newDepth = mix(depth_c, smoothed, dt);
    
    fragColor = vec4(newDepth);
}
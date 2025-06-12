#version 460 core

// Final shading pass for screen-space fluid rendering

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uTexture;

void main() {
    float depth = texture(uTexture, vTexCoord).r;
    
    // Skip background pixels (far plane)
    if (depth >= 0.999) {
        discard;
    }
    
    // Skip empty pixels (no particles)
    if (depth <= 0.001) {
        discard;
    }
    
    // Create beautiful water appearance
    vec3 deepWater = vec3(0.0, 0.1, 0.4);    // Deep blue
    vec3 shallowWater = vec3(0.1, 0.4, 0.8); // Light blue
    
    // Use depth for color variation
    float depthFactor = clamp(depth, 0.0, 1.0);
    vec3 waterColor = mix(shallowWater, deepWater, depthFactor);
    
    // Add some fresnel-like effects
    float fresnel = 1.0 - depthFactor;
    waterColor += vec3(0.1, 0.3, 0.5) * fresnel;
    
    // Output with transparency
    fragColor = vec4(waterColor, 0.8);
}
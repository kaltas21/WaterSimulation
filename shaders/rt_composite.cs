#version 460 core

// Real-time ray tracing compositing shader
// Combines reflections, refractions, and caustics

layout(local_size_x = 16, local_size_y = 16) in;

// Input textures
layout(binding = 0) uniform sampler2D uBaseColorTexture;   // Original rendered scene
layout(binding = 1) uniform sampler2D uReflectionTexture; // Ray traced reflections
layout(binding = 2) uniform sampler2D uRefractionTexture; // Ray traced refractions
layout(binding = 3) uniform sampler2D uCausticTexture;    // Ray traced caustics
layout(binding = 4) uniform sampler2D uDepthTexture;      // Depth buffer
layout(binding = 5) uniform sampler2D uNormalTexture;     // Surface normals

// Output texture
layout(rgba8, binding = 0) uniform image2D uFinalTexture;

// Uniforms
uniform vec3 uCameraPos;
uniform vec2 uResolution;
uniform float uReflectionStrength = 1.0;
uniform float uRefractionStrength = 1.0;
uniform float uCausticStrength = 1.0;
uniform bool uEnableReflections = true;
uniform bool uEnableRefractions = true;
uniform bool uEnableCaustics = true;
uniform bool uEnableSSR = true; // Screen space reflections fallback

// Water properties
uniform float uWaterIOR = 1.33;
uniform vec3 uWaterColor = vec3(0.1, 0.4, 0.7);
uniform float uWaterRoughness = 0.02;

// Post-processing
uniform float uExposure = 1.0;
uniform float uGamma = 2.2;
uniform bool uEnableToneMapping = true;

// Fresnel calculation
float fresnel(vec3 viewDir, vec3 normal, float ior) {
    float cosTheta = max(dot(viewDir, normal), 0.0);
    float eta = 1.0 / ior;
    float k = 1.0 - eta * eta * (1.0 - cosTheta * cosTheta);
    
    if (k < 0.0) return 1.0;
    
    float cosTheta2 = sqrt(k);
    float rs = (eta * cosTheta - cosTheta2) / (eta * cosTheta + cosTheta2);
    float rp = (cosTheta - eta * cosTheta2) / (cosTheta + eta * cosTheta2);
    
    return 0.5 * (rs * rs + rp * rp);
}

// Tone mapping
vec3 toneMap(vec3 color) {
    if (!uEnableToneMapping) return color;
    
    // Exposure
    color *= uExposure;
    
    // ACES tone mapping
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    
    color = (color * (a * color + b)) / (color * (c * color + d) + e);
    
    // Gamma correction
    color = pow(color, vec3(1.0 / uGamma));
    
    return color;
}

// Temporal anti-aliasing helper
vec3 temporalBlend(vec3 currentColor, vec3 previousColor, float alpha) {
    return mix(previousColor, currentColor, alpha);
}

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    
    // Check bounds
    if (coord.x >= int(uResolution.x) || coord.y >= int(uResolution.y)) {
        return;
    }
    
    vec2 uv = (vec2(coord) + 0.5) / uResolution;
    
    // Sample input textures
    vec3 baseColor = texture(uBaseColorTexture, uv).rgb;
    vec3 reflectionColor = texture(uReflectionTexture, uv).rgb;
    vec3 refractionColor = texture(uRefractionTexture, uv).rgb;
    vec3 causticColor = texture(uCausticTexture, uv).rgb;
    float depth = texture(uDepthTexture, uv).r;
    vec3 normal = texture(uNormalTexture, uv).xyz;
    
    // Initialize final color with base, fallback to water color if no base texture
    // For water surfaces, always start with a visible water color
    vec3 finalColor = (length(baseColor) > 0.001) ? baseColor : uWaterColor * 0.8;
    
    // Skip ray tracing for background pixels (make transparent)
    if (depth >= 1.0) {
        imageStore(uFinalTexture, coord, vec4(0.0, 0.0, 0.0, 0.0));
        return;
    }
    
    // For water surface pixels, ensure we have a visible base
    // Start with a semi-transparent water color base
    finalColor = uWaterColor * 0.6; // Visible but not too opaque
    
    // Calculate view direction for Fresnel
    vec3 viewDir = normalize(uCameraPos - vec3(uv * 2.0 - 1.0, depth)); // Simplified
    float fresnelFactor = fresnel(viewDir, normal, uWaterIOR);
    
    // Composite reflections
    if (uEnableReflections && length(reflectionColor) > 0.001) {
        float reflectionWeight = fresnelFactor * uReflectionStrength;
        finalColor = mix(finalColor, reflectionColor, reflectionWeight);
    }
    
    // Composite refractions
    if (uEnableRefractions && length(refractionColor) > 0.001) {
        float refractionWeight = (1.0 - fresnelFactor) * uRefractionStrength;
        finalColor = mix(finalColor, refractionColor, refractionWeight);
    }
    
    // Add caustics (additive blending)
    if (uEnableCaustics && length(causticColor) > 0.001) {
        finalColor += causticColor * uCausticStrength;
    }
    
    // Water color tinting for underwater areas
    float waterDepthFactor = clamp(depth * 10.0, 0.0, 1.0); // Simplified depth calculation
    finalColor = mix(finalColor, finalColor * uWaterColor, waterDepthFactor * 0.3);
    
    // Apply surface roughness (subtle blur effect) - skip if no base texture
    if (uWaterRoughness > 0.001 && length(baseColor) > 0.001) {
        // Simple roughness approximation
        vec2 roughnessOffset = (normal.xy - 0.5) * uWaterRoughness * 0.01;
        vec3 roughColor = texture(uBaseColorTexture, uv + roughnessOffset).rgb;
        finalColor = mix(finalColor, roughColor, uWaterRoughness);
    }
    
    // Enhance water color characteristics
    finalColor = mix(finalColor, finalColor * vec3(0.9, 1.0, 1.1), 0.1); // Slight blue tint
    
    // Apply tone mapping and gamma correction
    finalColor = toneMap(finalColor);
    
    // Clamp to valid range
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    // Store final result with appropriate alpha for blending
    float alpha = 0.7; // Semi-transparent for blending with normal scene
    imageStore(uFinalTexture, coord, vec4(finalColor, alpha));
}
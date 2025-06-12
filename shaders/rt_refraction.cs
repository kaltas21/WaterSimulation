#version 460 core

// Real-time ray traced refractions for underwater visibility

layout(local_size_x = 16, local_size_y = 16) in;

// Input textures
layout(binding = 0) uniform sampler2D uPositionTexture;
layout(binding = 1) uniform sampler2D uNormalTexture;
layout(binding = 2) uniform sampler2D uDepthTexture;
layout(binding = 3) uniform sampler2D uUnderwaterTexture; // Pre-rendered underwater scene

// Output texture
layout(rgba16f, binding = 1) uniform image2D uRefractionTexture;

// Uniforms
uniform vec3 uCameraPos;
uniform vec2 uResolution;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat4 uInverseViewMatrix;
uniform mat4 uInverseProjectionMatrix;

// Refraction properties
uniform float uWaterIOR = 1.33;
uniform float uWaterDepth = 5.0;
uniform vec3 uWaterColor = vec3(0.1, 0.3, 0.6);
uniform float uWaterAbsorption = 0.1;
uniform float uRefractionStrength = 1.0;

// Ray marching settings
uniform int uMaxRaySteps = 32;
uniform float uRayStepSize = 0.2;

// Snell's law refraction
vec3 refract2(vec3 incident, vec3 normal, float eta) {
    float cosI = -dot(normal, incident);
    float sinT2 = eta * eta * (1.0 - cosI * cosI);
    
    if (sinT2 > 1.0) {
        return vec3(0.0); // Total internal reflection
    }
    
    float cosT = sqrt(1.0 - sinT2);
    return eta * incident + (eta * cosI - cosT) * normal;
}

// Screen space refraction ray marching
vec3 screenSpaceRefraction(vec3 worldPos, vec3 normal, vec3 viewDir) {
    // Calculate refraction ray using Snell's law
    float eta = 1.0 / uWaterIOR; // Air to water
    vec3 refractionDir = refract2(viewDir, normal, eta);
    
    if (length(refractionDir) < 0.001) {
        // Total internal reflection - return water color
        return uWaterColor;
    }
    
    // Convert to screen space
    vec4 startClip = uProjectionMatrix * uViewMatrix * vec4(worldPos, 1.0);
    vec3 startNDC = startClip.xyz / startClip.w;
    vec2 startScreen = (startNDC.xy + 1.0) * 0.5;
    
    // Ray end point
    vec3 endWorld = worldPos + refractionDir * uWaterDepth;
    vec4 endClip = uProjectionMatrix * uViewMatrix * vec4(endWorld, 1.0);
    vec3 endNDC = endClip.xyz / endClip.w;
    vec2 endScreen = (endNDC.xy + 1.0) * 0.5;
    
    // Ray marching in screen space
    vec2 rayDir = endScreen - startScreen;
    float rayLength = length(rayDir);
    rayDir = normalize(rayDir);
    
    vec2 currentPos = startScreen;
    float travelDistance = 0.0;
    
    for (int i = 0; i < uMaxRaySteps; i++) {
        currentPos += rayDir * uRayStepSize * (1.0 / uResolution);
        travelDistance += uRayStepSize;
        
        // Check bounds
        if (currentPos.x < 0.0 || currentPos.x > 1.0 || 
            currentPos.y < 0.0 || currentPos.y > 1.0) {
            break;
        }
        
        // Sample underwater scene or bottom
        vec3 underwaterColor = texture(uUnderwaterTexture, currentPos).rgb;
        
        // Apply water color absorption based on travel distance
        float absorption = exp(-uWaterAbsorption * travelDistance);
        vec3 finalColor = mix(uWaterColor, underwaterColor, absorption);
        
        return finalColor;
    }
    
    // Default water color if no hit
    return uWaterColor;
}

// Caustic pattern generation (simplified)
float causticPattern(vec2 uv, float time) {
    vec2 p = uv * 8.0;
    
    // Multiple octaves of noise for realistic caustics
    float caustic = 0.0;
    caustic += sin(p.x * 2.0 + time * 2.0) * sin(p.y * 1.5 + time * 1.5) * 0.5;
    caustic += sin(p.x * 3.0 - time * 3.0) * sin(p.y * 2.5 - time * 2.0) * 0.3;
    caustic += sin(p.x * 5.0 + time * 4.0) * sin(p.y * 4.0 + time * 3.0) * 0.2;
    
    return max(0.0, caustic) * 0.5 + 0.5;
}

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    
    // Check bounds
    if (coord.x >= int(uResolution.x) || coord.y >= int(uResolution.y)) {
        return;
    }
    
    vec2 uv = (vec2(coord) + 0.5) / uResolution;
    
    // Sample G-Buffer
    vec3 worldPos = texture(uPositionTexture, uv).xyz;
    vec3 normal = texture(uNormalTexture, uv).xyz;
    float depth = texture(uDepthTexture, uv).r;
    
    // Skip background pixels
    if (depth >= 1.0) {
        imageStore(uRefractionTexture, coord, vec4(0.0));
        return;
    }
    
    // Calculate view direction
    vec3 viewDir = normalize(worldPos - uCameraPos);
    
    // Perform screen space refraction
    vec3 refractionColor = screenSpaceRefraction(worldPos, normal, viewDir);
    
    // Add caustic effects
    float time = gl_GlobalInvocationID.x * 0.01 + gl_GlobalInvocationID.y * 0.01; // Pseudo time
    float caustics = causticPattern(uv, time);
    refractionColor += vec3(caustics * 0.2, caustics * 0.3, caustics * 0.1);
    
    // Apply refraction strength
    refractionColor *= uRefractionStrength;
    
    // Store result
    imageStore(uRefractionTexture, coord, vec4(refractionColor, 1.0));
}
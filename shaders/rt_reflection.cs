#version 460 core

// Real-time ray traced reflections for water surface


layout(local_size_x = 16, local_size_y = 16) in;

// Input textures from G-Buffer
layout(binding = 0) uniform sampler2D uPositionTexture;
layout(binding = 1) uniform sampler2D uNormalTexture;
layout(binding = 2) uniform sampler2D uDepthTexture;
layout(binding = 3) uniform samplerCube uEnvironmentMap;

// Output texture
layout(rgba16f, binding = 0) uniform image2D uReflectionTexture;

// Uniforms
uniform vec3 uCameraPos;
uniform vec3 uLightPos;
uniform vec2 uResolution;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat4 uInverseViewMatrix;
uniform mat4 uInverseProjectionMatrix;

// Ray tracing quality settings
uniform int uMaxRaySteps = 64;
uniform float uRayStepSize = 0.1;
uniform float uReflectionStrength = 1.0;
uniform bool uUseFresnel = true;

// Water properties
uniform float uWaterIOR = 1.33;
uniform float uWaterRoughness = 0.02;

// Random number generation for noise
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Fresnel reflection calculation
float fresnel(vec3 viewDir, vec3 normal, float ior) {
    float cosTheta = max(dot(viewDir, normal), 0.0);
    float eta = 1.0 / ior;
    float k = 1.0 - eta * eta * (1.0 - cosTheta * cosTheta);
    
    if (k < 0.0) return 1.0; // Total internal reflection
    
    float cosTheta2 = sqrt(k);
    float rs = (eta * cosTheta - cosTheta2) / (eta * cosTheta + cosTheta2);
    float rp = (cosTheta - eta * cosTheta2) / (cosTheta + eta * cosTheta2);
    
    return 0.5 * (rs * rs + rp * rp);
}

// Screen space ray marching
vec3 screenSpaceReflection(vec3 worldPos, vec3 normal, vec3 viewDir) {
    // Calculate reflection ray
    vec3 reflectionDir = reflect(-viewDir, normal);
    
    // Convert to screen space
    vec4 startClip = uProjectionMatrix * uViewMatrix * vec4(worldPos, 1.0);
    vec3 startNDC = startClip.xyz / startClip.w;
    vec2 startScreen = (startNDC.xy + 1.0) * 0.5;
    
    // Ray end point
    vec3 endWorld = worldPos + reflectionDir * 10.0; // Max reflection distance
    vec4 endClip = uProjectionMatrix * uViewMatrix * vec4(endWorld, 1.0);
    vec3 endNDC = endClip.xyz / endClip.w;
    vec2 endScreen = (endNDC.xy + 1.0) * 0.5;
    
    // Ray marching in screen space
    vec2 rayDir = endScreen - startScreen;
    float rayLength = length(rayDir);
    rayDir = normalize(rayDir);
    
    vec2 currentPos = startScreen;
    float currentDepth = startNDC.z;
    
    for (int i = 0; i < uMaxRaySteps; i++) {
        currentPos += rayDir * uRayStepSize * (1.0 / uResolution);
        
        // Check bounds
        if (currentPos.x < 0.0 || currentPos.x > 1.0 || 
            currentPos.y < 0.0 || currentPos.y > 1.0) {
            break;
        }
        
        // Sample depth buffer
        float sampledDepth = texture(uDepthTexture, currentPos).r;
        
        // Check for intersection
        if (sampledDepth < currentDepth) {
            // Hit! Sample the position texture for color
            vec3 hitPos = texture(uPositionTexture, currentPos).xyz;
            
            // Simple shading calculation
            vec3 lightDir = normalize(uLightPos - hitPos);
            vec3 hitNormal = texture(uNormalTexture, currentPos).xyz;
            float NdotL = max(dot(hitNormal, lightDir), 0.0);
            
            // Return reflected color
            return vec3(0.2, 0.5, 0.8) * NdotL + vec3(0.1); // Water-like color
        }
        
        // Update depth along ray
        currentDepth += rayLength * uRayStepSize / float(uMaxRaySteps);
    }
    
    // No hit - sample environment map
    vec3 worldReflectionDir = mat3(uInverseViewMatrix) * reflectionDir;
    return texture(uEnvironmentMap, worldReflectionDir).rgb;
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
        imageStore(uReflectionTexture, coord, vec4(0.0));
        return;
    }
    
    // Calculate view direction
    vec3 viewDir = normalize(uCameraPos - worldPos);
    
    // Calculate Fresnel reflection coefficient
    float fresnelFactor = 1.0;
    if (uUseFresnel) {
        fresnelFactor = fresnel(viewDir, normal, uWaterIOR);
    }
    
    // Perform screen space reflection
    vec3 reflectionColor = screenSpaceReflection(worldPos, normal, viewDir);
    
    // Apply reflection strength and Fresnel
    reflectionColor *= uReflectionStrength * fresnelFactor;
    
    // Add some noise for realistic water surface
    float noise = random(uv + fract(sin(gl_GlobalInvocationID.x * 12.9898 + gl_GlobalInvocationID.y * 78.233) * 43758.5453));
    reflectionColor += (noise - 0.5) * 0.02; // Subtle noise
    
    // Store result
    imageStore(uReflectionTexture, coord, vec4(reflectionColor, 1.0));
}
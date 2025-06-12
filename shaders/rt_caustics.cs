#version 460 core

// Real-time ray traced caustics generation

layout(local_size_x = 16, local_size_y = 16) in;

// Input textures
layout(binding = 0) uniform sampler2D uWaterHeightMap;
layout(binding = 1) uniform sampler2D uWaterNormalMap;
layout(binding = 2) uniform sampler2D uDepthTexture;

// Output texture
layout(rgba16f, binding = 2) uniform image2D uCausticTexture;

// Uniforms
uniform vec3 uLightPos;
uniform vec3 uLightDir;
uniform vec2 uResolution;
uniform float uTime;
uniform float uWaterLevel = 0.0;
uniform float uCausticStrength = 1.0;
uniform float uWaterIOR = 1.33;

// Caustic generation parameters
uniform int uCausticRays = 64;
uniform float uCausticRadius = 2.0;
uniform float uFloorDepth = -5.0;

// Random number generation
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec2 random2(vec2 st) {
    st = vec2(dot(st, vec2(127.1, 311.7)),
              dot(st, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(st) * 43758.5453123);
}

// Simplex noise for realistic water surface variation
float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

// Generate water surface normal with multiple octaves
vec3 getWaterNormal(vec2 worldPos, float time) {
    vec2 p = worldPos * 0.1;
    
    // Multiple octaves for realistic water surface
    float h = 0.0;
    h += noise(p + time * 0.5) * 0.5;
    h += noise(p * 2.0 + time * 0.8) * 0.25;
    h += noise(p * 4.0 + time * 1.2) * 0.125;
    h += noise(p * 8.0 + time * 1.6) * 0.0625;
    
    // Calculate normal using gradient
    float eps = 0.01;
    float hx = noise(p + vec2(eps, 0.0) + time * 0.5) * 0.5;
    float hy = noise(p + vec2(0.0, eps) + time * 0.5) * 0.5;
    
    vec3 normal = normalize(vec3(-(hx - h) / eps, 1.0, -(hy - h) / eps));
    return normal;
}

// Refract light ray through water surface
vec3 refractRay(vec3 incident, vec3 normal, float eta) {
    float cosI = -dot(normal, incident);
    float sinT2 = eta * eta * (1.0 - cosI * cosI);
    
    if (sinT2 > 1.0) {
        return vec3(0.0); // Total internal reflection
    }
    
    float cosT = sqrt(1.0 - sinT2);
    return eta * incident + (eta * cosI - cosT) * normal;
}

// Trace caustic ray from light through water to floor
vec3 traceCausticRay(vec2 lightSurfacePos) {
    // Get water surface normal at this position
    vec3 waterNormal = getWaterNormal(lightSurfacePos, uTime);
    
    // Light ray direction (simplified directional light)
    vec3 lightRay = normalize(uLightDir);
    
    // Refract through water surface
    float eta = 1.0 / uWaterIOR; // Air to water
    vec3 refractedRay = refractRay(lightRay, waterNormal, eta);
    
    if (length(refractedRay) < 0.001) {
        return vec3(0.0); // Total internal reflection
    }
    
    // Trace to floor
    float t = (uFloorDepth - uWaterLevel) / refractedRay.y;
    if (t > 0.0) {
        vec3 floorHitPos = vec3(lightSurfacePos.x, uWaterLevel, lightSurfacePos.y) + refractedRay * t;
        return floorHitPos;
    }
    
    return vec3(0.0);
}

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    
    // Check bounds
    if (coord.x >= int(uResolution.x) || coord.y >= int(uResolution.y)) {
        return;
    }
    
    vec2 uv = (vec2(coord) + 0.5) / uResolution;
    
    // Convert screen coordinate to world space floor position
    vec2 floorPos = (uv - 0.5) * 20.0; // Assume 20x20 world space
    
    // Accumulate caustic intensity at this floor position
    float causticIntensity = 0.0;
    
    // Sample multiple light rays in a grid pattern above this floor position
    for (int i = 0; i < uCausticRays; i++) {
        // Generate sample positions around the floor position
        float angle = float(i) * 6.28318 / float(uCausticRays);
        vec2 offset = vec2(cos(angle), sin(angle)) * uCausticRadius * random(uv + float(i));
        vec2 lightSurfacePos = floorPos + offset;
        
        // Trace caustic ray
        vec3 hitPos = traceCausticRay(lightSurfacePos);
        
        if (hitPos.y <= uFloorDepth + 0.1) { // Hit the floor
            // Check if this ray contributes to the current floor position
            vec2 hitFloorPos = vec2(hitPos.x, hitPos.z);
            float distance = length(hitFloorPos - floorPos);
            
            if (distance < 0.5) { // Within caustic radius
                float intensity = 1.0 - (distance / 0.5);
                intensity = intensity * intensity; // Quadratic falloff
                causticIntensity += intensity;
            }
        }
    }
    
    // Normalize and apply strength
    causticIntensity /= float(uCausticRays);
    causticIntensity *= uCausticStrength;
    
    // Add temporal variation for animated caustics
    float timeVariation = sin(uTime * 2.0 + uv.x * 10.0) * sin(uTime * 1.5 + uv.y * 8.0);
    causticIntensity *= (1.0 + timeVariation * 0.3);
    
    // Clamp and convert to color
    causticIntensity = clamp(causticIntensity, 0.0, 2.0);
    
    // Caustic color (bluish-white)
    vec3 causticColor = vec3(0.8, 0.9, 1.0) * causticIntensity;
    
    // Store result
    imageStore(uCausticTexture, coord, vec4(causticColor, causticIntensity));
}
#version 460 core

in vec2 texCoord;

out vec4 fragColor;

// Water surface textures
uniform sampler2D smoothedWaterDepth;
uniform sampler2D waterNormalTexture;

// Scene textures
uniform sampler2D refractionTexture;
uniform sampler2D sceneDepthTexture;

// Environment
uniform samplerCube skyboxTexture;

// Matrices
uniform mat4 invViewProjMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

// Camera
uniform vec3 cameraPos;

// Water parameters
uniform vec3 waterColor = vec3(0.1, 0.3, 0.5);
uniform float refractionStrength = 0.02;
uniform float F0 = 0.02; // Fresnel at normal incidence for water
uniform float foamStart = 0.5;
uniform float foamEnd = 0.1;

vec3 reconstructWorldPos(vec2 uv, float depth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = invViewProjMatrix * clipPos;
    return worldPos.xyz / worldPos.w;
}

vec3 reconstructViewPos(vec2 uv, float viewDepth) {
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 clipPos = vec4(ndc, 0.0, 1.0);
    vec4 viewPos = inverse(projMatrix) * clipPos;
    viewPos.xyz /= viewPos.w;
    viewPos.z = -viewDepth;
    return viewPos.xyz;
}

void main() {
    float waterViewDepth = texture(smoothedWaterDepth, texCoord).r;
    
    // No water at this pixel
    if (waterViewDepth == 0.0) {
        fragColor = texture(refractionTexture, texCoord);
        return;
    }
    
    // Get water normal
    vec3 waterNormal = texture(waterNormalTexture, texCoord).xyz * 2.0 - 1.0;
    waterNormal = normalize(waterNormal);
    
    // Transform normal from view space to world space
    waterNormal = normalize((inverse(viewMatrix) * vec4(waterNormal, 0.0)).xyz);
    
    // Reconstruct water position
    vec3 waterViewPos = reconstructViewPos(texCoord, waterViewDepth);
    vec3 waterWorldPos = (inverse(viewMatrix) * vec4(waterViewPos, 1.0)).xyz;
    
    // View direction
    vec3 viewDir = normalize(cameraPos - waterWorldPos);
    
    // --- Refraction ---
    vec2 distortion = waterNormal.xz * refractionStrength;
    vec2 refractionUV = texCoord + distortion;
    refractionUV = clamp(refractionUV, 0.0, 1.0);
    
    vec3 refractionColor = texture(refractionTexture, refractionUV).rgb;
    
    // Apply water color absorption
    float waterDepth = waterViewDepth;
    float absorption = exp(-waterDepth * 0.1);
    refractionColor = mix(waterColor, refractionColor, absorption);
    
    // --- Reflection ---
    vec3 reflectDir = reflect(-viewDir, waterNormal);
    vec3 reflectionColor = texture(skyboxTexture, reflectDir).rgb;
    
    // --- Fresnel ---
    float NdotV = max(dot(waterNormal, viewDir), 0.0);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
    
    // --- Shoreline Foam ---
    float sceneDepth = texture(sceneDepthTexture, texCoord).r;
    vec3 sceneViewPos = reconstructViewPos(texCoord, sceneDepth);
    float depthDifference = abs(sceneViewPos.z - waterViewPos.z);
    
    float foamFactor = smoothstep(foamEnd, foamStart, depthDifference);
    vec3 foamColor = vec3(0.95, 0.98, 1.0);
    
    // --- Combine ---
    vec3 surfaceColor = mix(refractionColor, reflectionColor, fresnel);
    vec3 finalColor = mix(surfaceColor, foamColor, foamFactor);
    
    // Simple specular highlight
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    vec3 halfVector = normalize(viewDir + lightDir);
    float spec = pow(max(dot(waterNormal, halfVector), 0.0), 128.0);
    finalColor += vec3(1.0) * spec * 0.5;
    
    fragColor = vec4(finalColor, 1.0);
}
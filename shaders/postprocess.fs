#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform float time;
uniform vec2 resolution;

// Post-processing effects
uniform bool enableBloom;
uniform bool enableSSR; // Screen-space reflections
uniform bool enableDOF; // Depth of field
uniform bool enableVolumetricLighting;

// Screen-space refraction
uniform sampler2D refractionTexture;
uniform float refractionStrength;

// Bloom parameters
uniform float bloomThreshold;
uniform float bloomIntensity;

// DOF parameters
uniform float focusDistance;
uniform float focusRange;

// Volumetric lighting parameters
uniform vec3 lightPos;
uniform vec3 cameraPos;

// Simple box blur for bloom
vec3 boxBlur(sampler2D tex, vec2 uv, vec2 texelSize, int radius) {
    vec3 result = vec3(0.0);
    float count = 0.0;
    
    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(tex, uv + offset).rgb;
            count += 1.0;
        }
    }
    
    return result / count;
}

// Extract bright areas for bloom
vec3 extractBrights(vec3 color) {
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return color * max(0.0, brightness - bloomThreshold);
}

// Screen-space refraction effect
vec3 screenSpaceRefraction(vec2 uv, vec3 normal) {
    // Calculate refraction offset based on normal
    vec2 refractionOffset = normal.xy * refractionStrength;
    vec2 refractionUV = uv + refractionOffset;
    
    // Sample refracted color
    if (refractionUV.x >= 0.0 && refractionUV.x <= 1.0 && 
        refractionUV.y >= 0.0 && refractionUV.y <= 1.0) {
        return texture(refractionTexture, refractionUV).rgb;
    }
    
    return texture(screenTexture, uv).rgb;
}

// Simple depth of field
vec3 depthOfField(vec2 uv) {
    float depth = texture(depthTexture, uv).r;
    
    // Convert depth to linear distance
    float linearDepth = (2.0 * 0.1) / (100.0 + 0.1 - depth * (100.0 - 0.1));
    
    // Calculate blur amount based on distance from focus
    float blur = abs(linearDepth - focusDistance) / focusRange;
    blur = clamp(blur, 0.0, 1.0);
    
    // Sample with varying blur
    vec2 texelSize = 1.0 / resolution;
    int blurRadius = int(blur * 5.0);
    
    return boxBlur(screenTexture, uv, texelSize, blurRadius);
}

// Volumetric lighting
float volumetricLighting(vec2 uv) {
    // Convert light position to screen space
    vec2 lightScreen = vec2(0.5, 0.8); // Assume light is at top center
    
    vec2 deltaTexCoord = (uv - lightScreen);
    deltaTexCoord *= 1.0 / 100.0; // Number of samples
    
    float illuminationDecay = 1.0;
    vec2 coord = uv;
    float density = 0.0;
    
    for (int i = 0; i < 100; i++) {
        coord -= deltaTexCoord;
        vec3 sampleColor = texture(screenTexture, coord).rgb;
        float sampleBrightness = dot(sampleColor, vec3(0.33));
        
        density += sampleBrightness * illuminationDecay;
        illuminationDecay *= 0.96;
    }
    
    return density / 100.0;
}

void main() {
    vec2 uv = TexCoord;
    vec3 color = texture(screenTexture, uv).rgb;
    
    // Apply depth of field if enabled
    if (enableDOF) {
        color = depthOfField(uv);
    }
    
    // Apply bloom if enabled
    if (enableBloom) {
        vec2 texelSize = 1.0 / resolution;
        vec3 bloom = boxBlur(screenTexture, uv, texelSize * 2.0, 3);
        bloom = extractBrights(bloom);
        color += bloom * bloomIntensity;
    }
    
    // Apply volumetric lighting if enabled
    if (enableVolumetricLighting) {
        float volumetric = volumetricLighting(uv);
        color += vec3(0.8, 0.9, 1.0) * volumetric * 0.5;
    }
    
    // Tone mapping (simple Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}
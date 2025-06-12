#version 460

// Local work group size
layout(local_size_x = 16, local_size_y = 16) in;

// Wave height texture (read/write)
layout(r32f, binding = 0) uniform image2D waveHeightTexture;

// Wave parameters
uniform float time;
uniform int waveCount;
uniform vec4 waveParams[16]; // direction.xy, amplitude, wavelength
uniform vec4 waveParams2[16]; // speed, steepness, phase, unused

// Simulation parameters
uniform vec2 textureSize;
uniform float worldSize;

// Gerstner wave function
float gerstnerWave(vec2 position, vec2 direction, float amplitude, float wavelength, float speed, float steepness, float phase) {
    float k = 2.0 * 3.14159 / wavelength;
    float w = sqrt(9.8 * k);
    float phi = k * dot(direction, position) - w * speed * time + phase;
    
    // Gerstner wave with steepness
    float height = amplitude * sin(phi);
    
    // Add steepness effect (horizontal displacement affects height)
    vec2 tangent = steepness * amplitude * k * direction * cos(phi);
    height += length(tangent) * 0.1;
    
    return height;
}

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    
    // Check bounds
    if (coord.x >= textureSize.x || coord.y >= textureSize.y) {
        return;
    }
    
    // Convert texture coordinates to world position
    vec2 uv = vec2(coord) / textureSize;
    vec2 worldPos = (uv - 0.5) * worldSize;
    
    // Calculate total wave height
    float totalHeight = 0.0;
    
    for (int i = 0; i < waveCount && i < 16; i++) {
        vec2 direction = normalize(waveParams[i].xy);
        float amplitude = waveParams[i].z;
        float wavelength = waveParams[i].w;
        float speed = waveParams2[i].x;
        float steepness = waveParams2[i].y;
        float phase = waveParams2[i].z;
        
        totalHeight += gerstnerWave(worldPos, direction, amplitude, wavelength, speed, steepness, phase);
    }
    
    // Write height to texture
    imageStore(waveHeightTexture, coord, vec4(totalHeight, 0.0, 0.0, 1.0));
}
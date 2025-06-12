#version 460 core

// Upsampling compute shader for ray tracing resolution scaling
// Bilinear upsampling from low resolution to full resolution

layout(local_size_x = 16, local_size_y = 16) in;

// Input and output textures
layout(binding = 0) uniform sampler2D uLowResTexture;
layout(rgba8, binding = 0) restrict writeonly uniform image2D uHighResTexture;

uniform vec2 uLowResolution;
uniform vec2 uHighResolution;
uniform float uSharpenAmount = 0.2;

// Bicubic interpolation for higher quality upsampling
vec3 bicubicSample(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec2 coord = uv / texelSize - 0.5;
    vec2 fcoord = fract(coord);
    coord -= fcoord;
    
    vec3 c00 = texture(tex, (coord + vec2(0.0, 0.0)) * texelSize).rgb;
    vec3 c10 = texture(tex, (coord + vec2(1.0, 0.0)) * texelSize).rgb;
    vec3 c01 = texture(tex, (coord + vec2(0.0, 1.0)) * texelSize).rgb;
    vec3 c11 = texture(tex, (coord + vec2(1.0, 1.0)) * texelSize).rgb;
    
    vec3 cx0 = mix(c00, c10, fcoord.x);
    vec3 cx1 = mix(c01, c11, fcoord.x);
    
    return mix(cx0, cx1, fcoord.y);
}

// Sharpening filter
vec3 sharpen(sampler2D tex, vec2 uv, vec2 texelSize, float amount) {
    vec3 center = texture(tex, uv).rgb;
    vec3 blur = (
        texture(tex, uv + vec2(-texelSize.x, 0.0)).rgb +
        texture(tex, uv + vec2(texelSize.x, 0.0)).rgb +
        texture(tex, uv + vec2(0.0, -texelSize.y)).rgb +
        texture(tex, uv + vec2(0.0, texelSize.y)).rgb
    ) * 0.25;
    
    return center + (center - blur) * amount;
}

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    
    if (coord.x >= int(uHighResolution.x) || coord.y >= int(uHighResolution.y)) {
        return;
    }
    
    // Calculate UV coordinates for low resolution texture
    vec2 uv = (vec2(coord) + 0.5) / uHighResolution;
    vec2 lowResTexelSize = 1.0 / uLowResolution;
    
    // Perform bicubic upsampling
    vec3 upsampledColor = bicubicSample(uLowResTexture, uv, lowResTexelSize);
    
    // Apply sharpening to reduce blur from upsampling
    if (uSharpenAmount > 0.0) {
        upsampledColor = sharpen(uLowResTexture, uv, lowResTexelSize, uSharpenAmount);
    }
    
    // Store result
    imageStore(uHighResTexture, coord, vec4(upsampledColor, 1.0));
}
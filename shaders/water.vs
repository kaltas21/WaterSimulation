#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 ClipSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time; // Animation time
uniform bool enableMicroWaves; // Control micro-detail waves
uniform vec2 flowVelocity; // Water flow velocity
uniform float flowOffset; // Time-based flow offset

// Noise function for additional micro-detail
float noise(vec2 uv) {
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    // Copy original position
    vec3 pos = aPos;
    vec3 normal = aNormal;
    
    // Apply flow displacement
    vec2 flowDisplacement = flowVelocity * flowOffset;
    
    // Add flow-based height variation
    float flowHeight = 0.02 * sin(aPos.x * 3.0 - flowDisplacement.x * 5.0) * sin(aPos.z * 3.0 - flowDisplacement.y * 5.0);
    pos.y += flowHeight;
    
    // Add micro-detail waves based on position and time only if enabled
    if (enableMicroWaves) {
        // Modify micro-detail with flow
        float microDetail = 0.05 * sin((aPos.x - flowDisplacement.x) * 5.0 + time * 2.0) * sin((aPos.z - flowDisplacement.y) * 5.0 + time * 1.5);
        float microDetail2 = 0.03 * sin((aPos.x - flowDisplacement.x * 0.5) * 8.0 + time * 1.7) * sin((aPos.z - flowDisplacement.y * 0.5) * 7.0 + time * 2.3);
        
        // Add micro-detail to position
        pos.y += microDetail + microDetail2;
        
        // Update normal based on micro-detail slopes and flow
        normal.x += 0.2 * cos((aPos.x - flowDisplacement.x) * 5.0 + time * 2.0) * sin((aPos.z - flowDisplacement.y) * 5.0 + time * 1.5) + flowVelocity.x * 0.1;
        normal.z += 0.2 * sin((aPos.x - flowDisplacement.x) * 5.0 + time * 2.0) * cos((aPos.z - flowDisplacement.y) * 5.0 + time * 1.5) + flowVelocity.y * 0.1;
        normal = normalize(normal);
        
        // Pass texture coordinates with flow-based distortion
        TexCoord = aTexCoord + vec2(sin(time * 0.5 + aPos.x), cos(time * 0.7 + aPos.z)) * 0.01 + flowVelocity * 0.02;
    } else {
        // Without micro-detail, still apply flow distortion to texture
        TexCoord = aTexCoord + flowVelocity * 0.02;
    }
    
    // Calculate world-space position
    FragPos = vec3(model * vec4(pos, 1.0));
    Normal = mat3(transpose(inverse(model))) * normal;
    
    // Calculate final position
    ClipSpace = projection * view * model * vec4(pos, 1.0);
    gl_Position = ClipSpace;
} 
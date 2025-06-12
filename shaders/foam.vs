#version 460 core

layout (location = 0) in vec3 aPos;

out vec2 TexCoord;
out float Alpha;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 foamPosition;
uniform float foamSize;
uniform float foamLifetime; // 0-1 normalized

void main() {
    // Billboard the foam particle
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);
    
    vec3 worldPos = foamPosition + 
                    cameraRight * aPos.x * foamSize + 
                    cameraUp * aPos.y * foamSize;
    
    gl_Position = projection * view * vec4(worldPos, 1.0);
    
    // Pass texture coordinates
    TexCoord = aPos.xy + 0.5;
    
    // Fade out based on lifetime
    Alpha = foamLifetime;
}
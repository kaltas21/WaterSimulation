#version 460 core

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aPos;
    
    // Remove translation from view matrix for skybox
    mat4 rotView = mat4(mat3(view));
    vec4 pos = projection * rotView * vec4(aPos, 1.0);
    
    // Trick to make skybox always render behind everything else
    gl_Position = pos.xyww;
}
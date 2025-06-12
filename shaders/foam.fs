#version 460 core

in vec2 TexCoord;
in float Alpha;

out vec4 FragColor;

void main() {
    // Calculate distance from center for circular shape
    vec2 center = vec2(0.5, 0.5);
    float dist = length(TexCoord - center);
    
    // Create soft circular foam particle
    float alpha = smoothstep(0.5, 0.3, dist) * Alpha;
    
    // White foam color with transparency
    vec3 foamColor = vec3(0.95, 0.95, 0.95);
    
    FragColor = vec4(foamColor, alpha);
}
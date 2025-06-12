#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 viewPos;
uniform samplerCube skybox;

// Glass properties
uniform float glassTransparency = 0.15;  // Increased transparency
uniform vec3 glassColor = vec3(0.95, 0.95, 1.0);  // Slightly bluer
uniform float glassRefractionIndex = 1.05;  // Less refraction for better visibility

// Lighting properties
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;

void main() {
    // Normalize vectors
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Make normal face the viewer for double-sided transparency
    if(dot(viewDir, norm) < 0.0)
        norm = -norm;

    // Reduced fresnel effect
    float fresnel = 0.01 + 0.3 * pow(1.0 - abs(dot(viewDir, norm)), 3.0);
    
    // Calculate reflection and refraction vectors
    vec3 reflectDir = reflect(-viewDir, norm);
    vec3 refractDir = refract(-viewDir, norm, 1.0 / glassRefractionIndex);
    
    // Sample skybox for reflections and refractions
    vec3 reflectionColor = texture(skybox, reflectDir).rgb;
    vec3 refractionColor = texture(skybox, refractDir).rgb;
    
    // Lighting calculations
    // Ambient
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Mix reflection and refraction based on fresnel
    vec3 glassEffect = mix(refractionColor, reflectionColor, fresnel);
    
    // Combine lighting, glass color, and reflection/refraction
    // Reduce the influence of the glass color for more transparency
    vec3 result = (ambient + diffuse) * glassColor * 0.3 + specular + glassEffect;
    
    // Final color with fixed transparency value
    FragColor = vec4(result, glassTransparency);
} 
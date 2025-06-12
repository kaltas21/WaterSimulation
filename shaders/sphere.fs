#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 sphereColor;
uniform vec3 viewPos;
uniform bool useTexture;
uniform sampler2D sphereTexture;
uniform samplerCube skybox;
uniform bool enableReflections;
uniform float reflectivity;

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
    
    // Get base color from texture or uniform
    vec3 baseColor;
    float metallic = 0.0;
    if (useTexture) {
        baseColor = texture(sphereTexture, TexCoord).rgb;
        // Steel is highly metallic
        metallic = 1.0; // Maximum metallic value
        specular *= 1.5; // Moderate specular for realistic metal
    } else {
        baseColor = sphereColor;
    }
    
    // Calculate reflection if enabled
    vec3 reflectionColor = vec3(0.0);
    if (enableReflections) {
        // Calculate reflection vector (world space)
        vec3 I = normalize(FragPos - viewPos);
        vec3 R = reflect(I, norm);
        
        // Sample from environment map
        reflectionColor = texture(skybox, R).rgb;
        
    }
    
    vec3 result;
    if (enableReflections && useTexture) {
        // Mirror-like material
        float baseMix = 0.05; // Only 5% base color for tinting
        vec3 baseContribution = (ambient * 0.1 + diffuse * 0.05) * baseColor;
        
        // Strong specular highlights
        vec3 specularContribution = specular * 2.0;
        
        // Mix based on reflectivity
        result = mix(baseContribution + specularContribution, 
                    reflectionColor + specularContribution, 
                    reflectivity);
    } else {
        // Non-reflective or non-metallic material
        result = (ambient + diffuse) * baseColor + specular;
    }
    
    // Final color
    FragColor = vec4(result, 1.0);
} 
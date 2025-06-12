#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;
uniform sampler2D causticTexture;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float time;

// Calculate shadow with PCF
float calculateShadow(vec4 fragPosLightSpace) {
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Calculate bias to prevent shadow acne
    vec3 normal = normalize(Normal);
    vec3 lightColor = normalize(lightPos - FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightColor)), 0.005);
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

// Calculate underwater caustics
vec3 calculateCaustics(vec3 worldPos) {
    // Multiple layers of caustics for more realism
    vec2 causticCoord1 = worldPos.xz * 0.15 + time * 0.02;
    vec2 causticCoord2 = worldPos.xz * 0.25 - time * 0.03;
    
    vec3 caustic1 = texture(causticTexture, causticCoord1).rgb;
    vec3 caustic2 = texture(causticTexture, causticCoord2).rgb;
    
    // Combine caustics
    vec3 caustic = (caustic1 + caustic2) * 0.5;
    
    // Enhance contrast
    caustic = pow(caustic, vec3(1.5));
    
    return caustic;
}

void main() {
    vec3 color = texture(diffuseTexture, TexCoord).rgb;
    vec3 normal = normalize(Normal);
    vec3 ambient = 0.15 * color;
    
    // Diffuse lighting
    vec3 lightColor = normalize(lightPos - FragPos);
    float diff = max(dot(lightColor, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightColor, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightColor + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;
    
    // Calculate shadow
    float shadow = calculateShadow(FragPosLightSpace);
    
    // Calculate caustics (only in underwater areas)
    vec3 caustics = calculateCaustics(FragPos);
    
    // Apply caustics to areas that are lit (not in shadow)
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
    
    // Add caustics to lit areas with modulation
    lighting += caustics * (1.0 - shadow) * 0.8;
    
    vec3 result = lighting * color;
    
    FragColor = vec4(result, 1.0);
}
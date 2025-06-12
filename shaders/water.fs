#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 ClipSpace;

out vec4 FragColor;

// Water properties
uniform vec3 waterColor;
uniform float transparency;
uniform vec3 viewPos;
uniform samplerCube skybox;
uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D causticTex;
uniform sampler2D tileTexture;
uniform sampler2D waveHeightMap;

// Lighting properties
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;

// Physical constants
const float IOR_AIR = 1.0;
const float IOR_WATER = 1.333;
const float poolHeight = -5.0; // Floor level

// Caustics parameters
uniform float time; // For caustic animation

// Function to calculate underwater caustic effect
vec3 calculateCaustics(vec3 pos, float depth) {
    // Multiple layers of caustics with different scales and speeds
    vec2 causticCoord1 = pos.xz * 0.2 - time * 0.03;
    vec2 causticCoord2 = pos.xz * 0.3 - time * 0.05;
    vec2 causticCoord3 = pos.xz * 0.4 - time * 0.07;
    
    vec3 caustic1 = texture(causticTex, causticCoord1).rgb;
    vec3 caustic2 = texture(causticTex, causticCoord2).rgb;
    vec3 caustic3 = texture(causticTex, causticCoord3).rgb;
    
    // Combine caustics with different weights
    vec3 combinedCaustic = caustic1 * 0.5 + caustic2 * 0.3 + caustic3 * 0.2;
    
    // Enhance contrast for sharper caustic patterns
    combinedCaustic = pow(combinedCaustic, vec3(2.0));
    
    // Fade caustics with depth, but keep them moderate near the surface
    float causticStrength = 0.8 * exp(-depth * 0.15);
    return combinedCaustic * causticStrength;
}

// Function to sample tile texture for walls/floor
vec3 getTileColor(vec3 pos) {
    vec2 tileCoord;
    
    // Check which surface we're on (floor or walls)
    if (abs(pos.y - poolHeight) < 0.01) {
        // Floor
        tileCoord = pos.xz * 0.2; // Scale for appropriate tile size
    } else if (abs(abs(pos.x) - 5.0) < 0.01) {
        // X walls
        tileCoord = pos.zy * 0.2;
    } else {
        // Z walls
        tileCoord = pos.xy * 0.2;
    }
    
    // Sample tile texture (using repeating texture)
    return texture(tileTexture, tileCoord).rgb;
}

// Ray-box intersection for refraction
vec2 rayBoxIntersect(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    vec3 invDir = 1.0 / rayDir;
    vec3 tMin = (boxMin - rayOrigin) * invDir;
    vec3 tMax = (boxMax - rayOrigin) * invDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

void main() {
    // Normalize vectors
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Improved Fresnel effect using Schlick's approximation
    float R0 = pow((IOR_AIR - IOR_WATER) / (IOR_AIR + IOR_WATER), 2.0);
    float fresnel = R0 + (1.0 - R0) * pow(1.0 - max(0.0, dot(viewDir, norm)), 5.0);
    
    // Reflection and refraction vectors
    vec3 reflectDir = reflect(-viewDir, norm);
    vec3 refractDir = refract(-viewDir, norm, IOR_AIR / IOR_WATER);
    
    // Sample reflection texture if available, otherwise use skybox
    vec2 ndcCoords = ClipSpace.xy / ClipSpace.w / 2.0 + 0.5;
    vec3 reflectionColor = mix(
        texture(skybox, reflectDir).rgb,
        texture(reflectionTexture, vec2(ndcCoords.x, 1.0 - ndcCoords.y)).rgb,
        0.8  // Blend factor - adjust as needed
    );
    
    // Calculate water depth
    float waterDepth = abs(FragPos.y - poolHeight);
    
    // Calculate underwater color
    vec3 underwaterColor = vec3(0.0);
    
    // If we have refraction, calculate the underwater view
    if (refractDir != vec3(0.0)) {
        // Define pool box bounds
        vec3 boxMin = vec3(-5.0, poolHeight, -5.0);
        vec3 boxMax = vec3(5.0, 5.0, 5.0);
        
        // Trace refracted ray through water to hit pool surfaces
        vec2 intersect = rayBoxIntersect(FragPos, refractDir, boxMin, boxMax);
        
        if (intersect.y > 0.0) {
            // Calculate intersection point
            vec3 hitPos = FragPos + refractDir * intersect.y;
            
            // Get tile color at intersection point
            vec3 tileColor = getTileColor(hitPos);
            
            // Apply underwater lighting (bluish tint and depth-based attenuation)
            float depthFactor = exp(-waterDepth * 0.1);
            vec3 waterTint = mix(waterColor, vec3(1.0), 0.5);
            
            // Apply caustics to the tile color
            vec3 causticEffect = calculateCaustics(hitPos, waterDepth);
            
            // Combine caustics with tile color, affected by water depth and tint
            underwaterColor = mix(tileColor * depthFactor, causticEffect, 0.5) * waterTint;
        }
    }
    
    // Standard lighting calculations for water surface
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess * 4.0); // Sharper highlights
    vec3 specular = specularStrength * spec * lightColor;
    
    // Water surface sparkles (tiny, sharp specular highlights based on time)
    float sparkles = pow(max(dot(norm, halfwayDir), 0.0), 512.0) * 
                   (0.5 + 0.5 * sin(time * 5.0 + FragPos.x * 10.0 + FragPos.z * 10.0));
    vec3 sparkleColor = lightColor * sparkles;
    
    // Final color mixing based on Fresnel
    // Higher fresnel = more reflection, less refraction
    vec3 result = mix(underwaterColor, reflectionColor, fresnel);
    
    // Add highlights and sparkles (reduced intensity)
    result += specular * 0.3 + sparkleColor * 0.5;
    
    // Calculate transparency based on view angle (more transparent when looking straight down)
    float viewAngleTransparency = transparency * (1.0 - fresnel * 0.5);
    
    // Final color with angle-dependent transparency
    FragColor = vec4(result, viewAngleTransparency);
} 
#include "../include/WaterSurface.h"
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <random>
#include <omp.h>

// Static variable to track time since app start
static float g_totalTime = 0.0f;

WaterSurface::WaterSurface(int resolution, float size) 
    : resolution(resolution), size(size), waterColor(0.2f, 0.6f, 0.8f), transparency(0.7f),
      flowVelocity(0.0f, 0.0f), flowOffset(0.0f), foamBuffersInitialized(false) {
    
    // Initialize default wave
    WaveParam defaultWave;
    defaultWave.direction = glm::normalize(glm::vec2(1.0f, 1.0f));
    defaultWave.amplitude = 0.1f;
    defaultWave.wavelength = 4.0f;
    defaultWave.speed = 1.0f;
    defaultWave.steepness = 0.5f;
    waves.push_back(defaultWave);
}

WaterSurface::~WaterSurface() {
    // Clean up OpenGL objects
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    
    // Clean up foam buffers
    if (foamBuffersInitialized) {
        glDeleteVertexArrays(1, &foamVAO);
        glDeleteBuffers(1, &foamVBO);
    }
}

void WaterSurface::initialize() {
    generateMesh();
    generateIndices();
    
    // Create and bind VAO and VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void WaterSurface::generateMesh() {
    vertices.clear();
    float halfSize = size / 2.0f;
    float step = size / (float)(resolution - 1);
    
    for (int z = 0; z < resolution; z++) {
        for (int x = 0; x < resolution; x++) {
            float xPos = -halfSize + x * step;
            float zPos = -halfSize + z * step;
            float yPos = 0.0f;  // Will be updated during rendering
            
            // Position
            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
            
            // Normal (will be updated)
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
            
            // Texture coordinates
            vertices.push_back((float)x / (resolution - 1));
            vertices.push_back((float)z / (resolution - 1));
        }
    }
}

void WaterSurface::generateIndices() {
    indices.clear();
    
    for (int z = 0; z < resolution - 1; z++) {
        for (int x = 0; x < resolution - 1; x++) {
            unsigned int topLeft = z * resolution + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * resolution + x;
            unsigned int bottomRight = bottomLeft + 1;
            
            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}

glm::vec3 WaterSurface::calculateGerstnerWave(float x, float z, float time) {
    glm::vec3 result(x, 0.0f, z);
    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    
    // Optimize calculation by batching wave calculations
    // Pre-calculate wave phase and amplitude for each wave once
    const int maxWaves = waves.size();
    if (maxWaves > 0) {
        // Use stack memory for faster access
        const int MAX_WAVES = 16; // Reasonable limit for most use cases
        float phases[MAX_WAVES];
        float sinValues[MAX_WAVES];
        float cosValues[MAX_WAVES];
        glm::vec2 directions[MAX_WAVES];
        float amplitudes[MAX_WAVES];
        float steepnesses[MAX_WAVES];
        float waveNumbers[MAX_WAVES];
        
        int activeWaves = 0;
        
        // Pre-compute values for each wave to avoid redundant calculations
        for (int i = 0; i < std::min(maxWaves, MAX_WAVES); i++) {
            const auto& wave = waves[i];
            
            // Skip waves with zero amplitude
            if (std::abs(wave.amplitude) < 0.001f) {
                continue;
            }
            
            // Wave parameters
            directions[activeWaves] = glm::normalize(wave.direction);
            amplitudes[activeWaves] = wave.amplitude;
            steepnesses[activeWaves] = wave.steepness;
            
            // Calculate wave number and phase constant
            float k = 2.0f * glm::pi<float>() / wave.wavelength;
            waveNumbers[activeWaves] = k;
            
            float w = sqrt(9.8f * k);  // Angular frequency
            
            // Compute phase
            float dotProduct = directions[activeWaves].x * x + directions[activeWaves].y * z;
            phases[activeWaves] = k * dotProduct - wave.speed * w * time;
            
            // Precalculate trig functions (expensive operations)
            sinValues[activeWaves] = sin(phases[activeWaves]);
            cosValues[activeWaves] = cos(phases[activeWaves]);
            
            activeWaves++;
        }
        
        // Apply all waves at once using the pre-calculated values
        for (int i = 0; i < activeWaves; i++) {
            float A = amplitudes[i];
            float S = steepnesses[i];
            float k = waveNumbers[i];
            float sinPhase = sinValues[i];
            float cosPhase = cosValues[i];
            glm::vec2 D = directions[i];
            
            // Modified Gerstner wave formula for better horizontal/vertical balance
            float horizontalScale = S * 2.0f; // Amplify horizontal motion
            result.x += D.x * A * horizontalScale * cosPhase;
            result.y += A * sinPhase;
            result.z += D.y * A * horizontalScale * cosPhase;
            
            // Update normal (consistent with modified displacement)
            normal.x -= D.x * k * A * horizontalScale * cosPhase;
            normal.y -= S * k * A * sinPhase;
            normal.z -= D.y * k * A * horizontalScale * cosPhase;
        }
    }
    
    // Add ripples
    result.y += rippleHeight(x, z, time);
    
    // Store normalized normal
    normal = glm::normalize(normal);
    
    return result;
}

float WaterSurface::rippleHeight(float x, float z, float time) {
    float height = 0.0f;
    
    for (auto& ripple : ripples) {
        float dx = x - ripple.center.x;
        float dz = z - ripple.center.y;
        
        if (ripple.isDirectional) {
            // Directional wave propagation
            float distanceInDirection = dx * ripple.direction.x + dz * ripple.direction.y;
            float perpendicularDistance = abs(dx * ripple.direction.y - dz * ripple.direction.x);
            
            // Wave front position
            float waveDistance = distanceInDirection - ripple.speed * ripple.time;
            
            // Apply wave only in forward direction and within perpendicular bounds
            if (waveDistance >= 0 && waveDistance <= ripple.radius && perpendicularDistance < ripple.radius * 0.5f) {
                float factor = sin(waveDistance * (glm::pi<float>() / ripple.radius));
                float perpFactor = exp(-perpendicularDistance * 2.0f / ripple.radius); // Falloff perpendicular to direction
                float amplitude = ripple.amplitude * exp(-ripple.decay * ripple.time);
                height += factor * amplitude * perpFactor;
            }
        } else {
            // Radial wave propagation (original behavior)
            float distance = sqrt(dx*dx + dz*dz);
            float waveDistance = distance - ripple.speed * ripple.time;
            if (waveDistance >= 0 && waveDistance <= ripple.radius) {
                float factor = sin(waveDistance * (glm::pi<float>() / ripple.radius));
                float amplitude = ripple.amplitude * exp(-ripple.decay * ripple.time);
                height += factor * amplitude;
            }
        }
    }
    
    return height;
}

void WaterSurface::update(float deltaTime) {
    // Accumulate time for wave animation
    g_totalTime += deltaTime;
    
    // Update flow offset
    flowOffset += deltaTime;
    
    // Update flow impulses and dampen flow velocity
    flowVelocity *= 0.98f; // Gradual damping
    
    for (auto it = flowImpulses.begin(); it != flowImpulses.end();) {
        it->time += deltaTime;
        it->strength = std::max(0.0f, 1.0f - it->time * 2.0f); // Decay over 0.5 seconds
        
        if (it->strength <= 0.0f) {
            it = flowImpulses.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update ripples
    for (auto it = ripples.begin(); it != ripples.end();) {
        it->time += deltaTime;
        
        // Remove ripples that have decayed too much
        if (it->time > 5.0f) {
            it = ripples.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update foam particles
    updateFoam(deltaTime);
    
    // Update vertex positions and normals based on Gerstner waves
    std::vector<float> updatedVertices = vertices;
    float halfSize = size / 2.0f;
    float step = size / (float)(resolution - 1);
    float time = g_totalTime; // Use our accumulated time instead of glfwGetTime()

    // Use parallel processing if available (OpenMP)
    #pragma omp parallel for collapse(2) if(resolution > 50)
    for (int z = 0; z < resolution; z++) {
        for (int x = 0; x < resolution; x++) {
            int vertexIndex = (z * resolution + x) * 8;
            
            float xPos = -halfSize + x * step;
            float zPos = -halfSize + z * step;
            
            // Calculate new position with Gerstner waves - this is the performance-critical part
            glm::vec3 newPos = calculateGerstnerWave(xPos, zPos, time);
            
            // Update position
            updatedVertices[vertexIndex] = newPos.x;
            updatedVertices[vertexIndex + 1] = newPos.y;
            updatedVertices[vertexIndex + 2] = newPos.z;
            
            // Calculate normal by using cross product of adjacent vertices
            // Optimize: Calculate normals using finite differences
            // This avoids redundant wave calculations for neighboring vertices
            glm::vec3 dx, dz;
            
            // Use central differences when possible for better accuracy
            if (x > 0 && x < resolution - 1) {
                float prevX = -halfSize + (x - 1) * step;
                float nextX = -halfSize + (x + 1) * step;
                glm::vec3 prevPos = calculateGerstnerWave(prevX, zPos, time);
                glm::vec3 nextPos = calculateGerstnerWave(nextX, zPos, time);
                dx = (nextPos - prevPos) * 0.5f; // Central difference
            } else if (x < resolution - 1) {
                float nextX = -halfSize + (x + 1) * step;
                glm::vec3 nextPos = calculateGerstnerWave(nextX, zPos, time);
                dx = nextPos - newPos;
            } else {
                float prevX = -halfSize + (x - 1) * step;
                glm::vec3 prevPos = calculateGerstnerWave(prevX, zPos, time);
                dx = newPos - prevPos;
            }
            
            if (z > 0 && z < resolution - 1) {
                float prevZ = -halfSize + (z - 1) * step;
                float nextZ = -halfSize + (z + 1) * step;
                glm::vec3 prevPos = calculateGerstnerWave(xPos, prevZ, time);
                glm::vec3 nextPos = calculateGerstnerWave(xPos, nextZ, time);
                dz = (nextPos - prevPos) * 0.5f; // Central difference
            } else if (z < resolution - 1) {
                float nextZ = -halfSize + (z + 1) * step;
                glm::vec3 nextPos = calculateGerstnerWave(xPos, nextZ, time);
                dz = nextPos - newPos;
            } else {
                float prevZ = -halfSize + (z - 1) * step;
                glm::vec3 prevPos = calculateGerstnerWave(xPos, prevZ, time);
                dz = newPos - prevPos;
            }
            
            glm::vec3 normal = glm::normalize(glm::cross(dz, dx));
            
            // Update normal
            updatedVertices[vertexIndex + 3] = normal.x;
            updatedVertices[vertexIndex + 4] = normal.y;
            updatedVertices[vertexIndex + 5] = normal.z;
        }
    }
    
    // Update VBO with new vertex data - use GL_DYNAMIC_DRAW for better performance
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, updatedVertices.size() * sizeof(float), updatedVertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WaterSurface::render(unsigned int shaderProgram) {
    // Set flow uniforms
    glUniform2fv(glGetUniformLocation(shaderProgram, "flowVelocity"), 1, glm::value_ptr(flowVelocity));
    glUniform1f(glGetUniformLocation(shaderProgram, "flowOffset"), flowOffset);
    
    glBindVertexArray(VAO);
    
    // Draw water surface
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
}

void WaterSurface::addRipple(const glm::vec3& position, float magnitude) {
    Ripple ripple;
    ripple.center = glm::vec2(position.x, position.z);
    ripple.amplitude = magnitude * 0.3f; // Slightly higher amplitude
    ripple.radius = 2.5f; // Larger initial radius
    ripple.speed = 2.0f; // Higher speed
    ripple.decay = 1.5f; // Slower decay
    ripple.time = 0.0f;
    ripple.direction = glm::vec2(0.0f, 0.0f); // Radial wave
    ripple.isDirectional = false;
    
    ripples.push_back(ripple);
}

void WaterSurface::addDirectionalRipple(const glm::vec3& position, const glm::vec2& direction, float magnitude) {
    Ripple ripple;
    ripple.center = glm::vec2(position.x, position.z);
    ripple.amplitude = magnitude * 0.4f;
    ripple.radius = 3.0f;
    ripple.speed = 2.5f;
    ripple.decay = 1.2f;
    ripple.time = 0.0f;
    ripple.direction = glm::normalize(direction);
    ripple.isDirectional = true;
    
    ripples.push_back(ripple);
}

void WaterSurface::createSplash(const glm::vec3& position, float magnitude) {
    // Create multiple ripples with varying parameters to simulate a splash
    // Scale magnitude to be more proportional to impact speed - much higher for gravity drops
    float scaledMagnitude = std::min(magnitude * 0.25f, 1.2f); // Much higher scaling and maximum
    
    for (int i = 0; i < 3; i++) {
        Ripple ripple;
        ripple.center = glm::vec2(position.x, position.z);
        ripple.amplitude = scaledMagnitude * (1.0f - 0.1f * i); // Less falloff for stronger waves
        ripple.radius = 2.0f + i * 1.5f + scaledMagnitude * 0.8f; // Larger initial radius
        ripple.speed = 2.5f + i * 0.4f + scaledMagnitude * 0.3f;  // Higher speed
        ripple.decay = 1.8f - i * 0.1f;  // Slower decay for longer lasting waves
        ripple.time = 0.0f;
        ripple.direction = glm::vec2(0.0f, 0.0f); // Radial waves for splash
        ripple.isDirectional = false;
        
        ripples.push_back(ripple);
    }
    
    // Generate foam for splashes
    if (scaledMagnitude > 0.2f) {
        int foamCount = static_cast<int>(20 * scaledMagnitude);
        generateFoam(position, scaledMagnitude, foamCount);
    }
}

void WaterSurface::addImpulse(const glm::vec3& position, const glm::vec2& impulse, float radius) {
    FlowImpulse flowImpulse;
    flowImpulse.position = glm::vec2(position.x, position.z);
    flowImpulse.velocity = impulse;
    flowImpulse.radius = radius;
    flowImpulse.strength = 1.0f;
    flowImpulse.time = 0.0f;
    
    flowImpulses.push_back(flowImpulse);
    
    // Also temporarily affect the global flow
    flowVelocity += impulse * 0.1f;
}

void WaterSurface::generateFoam(const glm::vec3& position, float intensity, int count) {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * glm::pi<float>());
    std::uniform_real_distribution<float> speedDist(0.5f, 2.0f);
    std::uniform_real_distribution<float> sizeDist(0.02f, 0.08f);
    std::uniform_real_distribution<float> lifeDist(1.0f, 3.0f);
    
    for (int i = 0; i < count; i++) {
        FoamParticle foam;
        
        // Random position around the impact point
        float angle = angleDist(rng);
        float speed = speedDist(rng) * intensity;
        
        foam.position = position;
        foam.position.x += cos(angle) * 0.1f;
        foam.position.z += sin(angle) * 0.1f;
        
        // Random velocity with upward bias
        foam.velocity.x = cos(angle) * speed;
        foam.velocity.y = 0.5f + speedDist(rng) * 0.5f * intensity;
        foam.velocity.z = sin(angle) * speed;
        
        // Random lifetime and size
        foam.maxLifetime = lifeDist(rng);
        foam.lifetime = foam.maxLifetime;
        foam.size = sizeDist(rng) * (1.0f + intensity * 0.5f);
        
        foamParticles.push_back(foam);
    }
}

void WaterSurface::updateFoam(float deltaTime) {
    // Update existing foam particles
    for (auto it = foamParticles.begin(); it != foamParticles.end();) {
        // Update lifetime
        it->lifetime -= deltaTime;
        
        if (it->lifetime <= 0.0f) {
            it = foamParticles.erase(it);
            continue;
        }
        
        // Apply physics
        it->velocity.y -= 9.81f * deltaTime * 0.1f; // Light gravity
        it->position += it->velocity * deltaTime;
        
        // Apply drag
        it->velocity *= (1.0f - deltaTime * 2.0f);
        
        // Fade size based on lifetime
        float lifetimeRatio = it->lifetime / it->maxLifetime;
        it->size *= (0.95f + lifetimeRatio * 0.05f);
        
        ++it;
    }
}

void WaterSurface::renderFoam(unsigned int foamShader, const glm::mat4& view, const glm::mat4& projection) {
    if (foamParticles.empty()) return;
    
    // Initialize foam buffers if not already done
    if (!foamBuffersInitialized) {
        // Create a simple quad for billboarded particles
        float quadVertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f
        };
        
        glGenVertexArrays(1, &foamVAO);
        glGenBuffers(1, &foamVBO);
        
        glBindVertexArray(foamVAO);
        glBindBuffer(GL_ARRAY_BUFFER, foamVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindVertexArray(0);
        foamBuffersInitialized = true;
    }
    
    // Enable blending for foam
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write to depth buffer for transparent particles
    
    glUseProgram(foamShader);
    glUniformMatrix4fv(glGetUniformLocation(foamShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(foamShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glBindVertexArray(foamVAO);
    
    // Render each foam particle
    for (const auto& foam : foamParticles) {
        glUniform3fv(glGetUniformLocation(foamShader, "foamPosition"), 1, glm::value_ptr(foam.position));
        glUniform1f(glGetUniformLocation(foamShader, "foamSize"), foam.size);
        glUniform1f(glGetUniformLocation(foamShader, "foamLifetime"), foam.lifetime / foam.maxLifetime);
        
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
} 
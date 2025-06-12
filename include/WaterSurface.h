#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <random>

class WaterSurface {
public:
    WaterSurface(int resolution = 100, float size = 10.0f);
    ~WaterSurface();

    void initialize();
    void update(float deltaTime);
    void render(unsigned int shaderProgram);

    // Gerstner wave parameters
    struct WaveParam {
        glm::vec2 direction;
        float amplitude;
        float wavelength;
        float speed;
        float steepness;
    };
    
    // Foam particles
    struct FoamParticle {
        glm::vec3 position;
        glm::vec3 velocity;
        float lifetime;
        float maxLifetime;
        float size;
    };

    // Wave interactions
    void addRipple(const glm::vec3& position, float magnitude);
    void addDirectionalRipple(const glm::vec3& position, const glm::vec2& direction, float magnitude);
    void createSplash(const glm::vec3& position, float magnitude);

    // Water properties getters/setters
    void setColor(const glm::vec3& color) { waterColor = color; }
    glm::vec3 getColor() const { return waterColor; }
    void setTransparency(float alpha) { transparency = alpha; }
    float getTransparency() const { return transparency; }
    void addWave(const WaveParam& wave) { waves.push_back(wave); }
    void clearWaves() { waves.clear(); }
    std::vector<WaveParam>& getWaves() { return waves; }
    
    // Water flow/current
    void setFlowVelocity(const glm::vec2& velocity) { flowVelocity = velocity; }
    glm::vec2 getFlowVelocity() const { return flowVelocity; }
    void addImpulse(const glm::vec3& position, const glm::vec2& impulse, float radius);
    
    // Foam generation
    void generateFoam(const glm::vec3& position, float intensity, int count = 20);
    void updateFoam(float deltaTime);
    const std::vector<FoamParticle>& getFoamParticles() const { return foamParticles; }
    
    // For ray tracing integration
    unsigned int getVAO() const { return VAO; }
    int getVertexCount() const { return indices.size(); }

    // Foam rendering
    void renderFoam(unsigned int foamShader, const glm::mat4& view, const glm::mat4& projection);

private:
    // Geometry data
    unsigned int VAO, VBO, EBO;
    int resolution;
    float size;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Foam rendering data
    unsigned int foamVAO, foamVBO;
    bool foamBuffersInitialized;

    // Water properties
    glm::vec3 waterColor;
    float transparency;
    std::vector<WaveParam> waves;
    
    // Water flow
    glm::vec2 flowVelocity;
    float flowOffset;
    
    // Flow impulses (temporary flow disturbances)
    struct FlowImpulse {
        glm::vec2 position;
        glm::vec2 velocity;
        float radius;
        float strength;
        float time;
    };
    std::vector<FlowImpulse> flowImpulses;
    
    // Ripples
    struct Ripple {
        glm::vec2 center;
        float amplitude;
        float radius;
        float speed;
        float decay;
        float time;
        glm::vec2 direction; // Direction of wave propagation (0,0 = radial)
        bool isDirectional;
    };
    std::vector<Ripple> ripples;
    
    // Foam particles
    std::vector<FoamParticle> foamParticles;

    // Helper methods
    void generateMesh();
    void generateIndices();
    glm::vec3 calculateGerstnerWave(float x, float z, float time);
    float rippleHeight(float x, float z, float time);
}; 
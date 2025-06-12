#ifndef SPH_COMPUTE_SYSTEM_H
#define SPH_COMPUTE_SYSTEM_H

#define _USE_MATH_DEFINES // For M_PI in MSVC
#include <vector>
#include <cstdint>
#include <cmath> // Added for M_PI and other math functions
#include <glm/glm.hpp>
#include "GLResources.h" // For SPHParticleCompute if defined there, or define SPHParticleCompute here

namespace WaterSim {

// SPH particle structure
struct SPHParticleCompute {
    glm::vec3 position;
    float density;
    glm::vec3 velocity;
    float pressure;
};

// SPH constants
namespace SPHConstants {
    constexpr float PARTICLE_RADIUS = 0.0457f;       
    constexpr float KERNEL_RADIUS = PARTICLE_RADIUS * 4.0f; // h = 4 * r
    constexpr float CELL_SIZE = PARTICLE_RADIUS * 4.0f;     // Grid cell size
    constexpr float MASS = 0.02f;                     // Mass of each particle
    constexpr float REST_DENSITY = 998.27f;           // Rest density
    constexpr float STIFFNESS = 250.0f;               // Gas constant
    constexpr float REST_PRESSURE = 0.0f;             // Rest pressure
    constexpr float VIS_COEFF = 0.035f;               // Viscosity
    constexpr float DT = 0.0012f;                     // Time step
    const glm::vec3 GRAVITY = glm::vec3(0.0f, -9.81f, 0.0f); // Gravity

    // Kernel constants calculated
    constexpr float H2 = KERNEL_RADIUS * KERNEL_RADIUS;
    constexpr float H6 = H2 * H2 * H2;
    constexpr float H9 = H6 * H2 * KERNEL_RADIUS;
    constexpr float PI_VALUE = 3.14159265358979323846f;

    // Kernel constants calculated
    constexpr float POLY6_KERNEL_WEIGHT_CONST = 315.0f / (64.0f * PI_VALUE * H9);
    constexpr float SPIKY_KERNEL_WEIGHT_CONST = 15.0f / (PI_VALUE * H6);
    constexpr float VIS_KERNEL_WEIGHT_CONST = 45.0f / (PI_VALUE * H6);

    constexpr uint32_t INVALID_INDEX = 0xFFFFFFFFu; // Used for linked lists in grid cells
}

class SPHComputeSystem {
public:
    SPHComputeSystem();
    ~SPHComputeSystem();
    
    // Initialize with particle count
    bool initialize(uint32_t numParticles, const glm::vec3& boxMin, const glm::vec3& boxMax);
    
    // Update simulation
    void update(float deltaTime);
    
    // Render particles
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    // Reset simulation
    void reset();
    
    // Add particles
    void addParticles(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& velocities);
    
    // Sphere interaction
    void applyImpulse(const glm::vec3& position, const glm::vec3& impulse, float radius);
    
    // Getters
    uint32_t getParticleCount() const { return numParticles_; }
    const glm::vec3& getBoxMin() const { return boxMin_; }
    const glm::vec3& getBoxMax() const { return boxMax_; }
    
    // Rendering options
    enum ColorMode {
        COLOR_NORMAL = 0,
        COLOR_VELOCITY = 1,
        COLOR_DENSITY = 2,
        COLOR_PRESSURE = 3
    };
    
    void setColorMode(ColorMode mode) { colorMode_ = mode; }
    ColorMode getColorMode() const { return colorMode_; }
    
    // Gravity control
    void setGravity(const glm::vec3& gravity) { gravity_ = gravity; }
    const glm::vec3& getGravity() const { return gravity_; }
    
    // Enable/disable features
    void setUseFilteredViscosity(bool enable) { useFilteredViscosity_ = enable; }
    void setCurvatureFlowIterations(int iterations) { curvatureFlowIterations_ = iterations; }
    
    // Container rendering
    void setRenderContainer(bool render) { renderContainer_ = render; }
    bool getRenderContainer() const { return renderContainer_; }
    
    // Window resize handling
    void onWindowResize(int width, int height);
    
private:
    // Particle data
    uint32_t numParticles_;
    uint32_t maxParticles_;
    
    // Simulation bounds
    glm::vec3 boxMin_;
    glm::vec3 boxMax_;
    
    // Gravity direction
    glm::vec3 gravity_;
    
    // Sphere interaction data
    glm::vec3 spherePosition_;
    glm::vec3 sphereImpulse_;
    float sphereRadius_;
    bool sphereActive_;
    
    // Grid parameters
    float gridCellSize_;
    glm::vec3 gridOrigin_;
    glm::vec3 gridSize_;
    glm::ivec3 gridRes_;
    
    // OpenGL resources
    GLuint particleBuffers_[2]; // Double buffering
    GLuint particleVAO_;
    GLuint billboardIndexBuffer_; // Index buffer for billboard quads
    GLuint gridTexture_ = 0;
    GLuint counterBuffer_ = 0;  // For atomic counters
    GLuint velocityTexture_ = 0; // For filtered velocity field
    glm::uvec3 gridDim_ = glm::uvec3(0);
    
    // Billboard rendering for screen-space fluid
    GLuint billboardVAO_;
    
    // Shaders
    GLuint simStep1Program_;   // Position integration + grid population
    GLuint simStep2Program_;   // Grid offset calculation
    GLuint simStep3Program_;   // Particle reordering
    GLuint simStep4Program_;   // Velocity field calculation
    GLuint simStep5Program_;   // Density and pressure
    GLuint simStep6Program_;   // Force calculation
    GLuint renderProgram_;     // Particle rendering shader
    GLuint depthProgram_;      // Depth rendering for screen-space fluid
    GLuint smoothProgram_;     // Curvature flow smoothing
    GLuint finalProgram_;      // Final surface shading
    
    // Framebuffers for screen-space rendering
    GLuint depthFBO_;
    GLuint depthTexture_;
    GLuint smoothFBO_[2];
    GLuint smoothTexture_[2];
    int windowWidth_;
    int windowHeight_;
    
    // Container rendering
    GLuint containerVAO_;
    GLuint containerVBO_;
    GLuint containerEBO_;
    GLuint containerShader_;
    bool renderContainer_;
    
    // Current buffer index for double buffering
    int currentBuffer_;
    
    // Rendering options
    ColorMode colorMode_;
    bool useFilteredViscosity_;
    int curvatureFlowIterations_;
    
    // Timing
    float accumulatedTime_;
    
    // Smoothing result buffer tracking
    int finalSmoothedBuffer_;
    
    // Helper methods
    void initializeGrid();
    void createBuffers();
    void loadShaders();
    void createContainerGeometry();
    void createFramebuffers();
    
    void runSimulationPass(int pass);
    void swapBuffers();
    
    void renderParticles(const glm::mat4& view, const glm::mat4& projection);
    void renderParticlesAsPoints(const glm::mat4& view, const glm::mat4& projection);
    void renderGlassContainer(const glm::mat4& view, const glm::mat4& projection);
    
    // Screen-space fluid rendering pipeline
    void renderScreenSpaceFluid(const glm::mat4& view, const glm::mat4& projection);
    void renderParticleDepth(const glm::mat4& view, const glm::mat4& projection);
    void applyCurvatureFlowSmoothing();
    void renderFinalShading(const glm::mat4& view, const glm::mat4& projection);
};

} // namespace WaterSim

#endif // SPH_COMPUTE_SYSTEM_H
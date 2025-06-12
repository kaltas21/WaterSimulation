#pragma once

#include <glm/glm.hpp>
#include <memory>
#include "WaterSurface.h"
#include "SPHComputeSystem.h"
#include "Config.h"

namespace WaterSim {

enum class SimulationType {
    NONE,
    REGULAR_WATER,
    SPH_COMPUTE   // GPU-optimized SPH using compute shaders
};

class SimulationManager {
public:
    SimulationManager(const Config& config);
    ~SimulationManager();
    
    // Simulation control
    void setSimulationType(SimulationType type);
    SimulationType getCurrentType() const { return currentType_; }
    
    // Initialize the selected simulation
    void initialize();
    void cleanup();
    
    // Update simulation
    void update(float deltaTime);
    
    // Render simulation
    void render(const glm::mat4& view, const glm::mat4& projection, 
                unsigned int waterShader, bool rayTracingEnabled);
    
    // Water surface interactions (for regular water)
    void addRipple(const glm::vec3& position, float magnitude);
    void addDirectionalRipple(const glm::vec3& position, const glm::vec2& direction, float magnitude);
    void createSplash(const glm::vec3& position, float magnitude);
    void addWaterFlowImpulse(const glm::vec3& position, const glm::vec2& impulse, float radius);
    
    // SPH interactions
    void applyImpulse(const glm::vec3& position, const glm::vec3& impulse, float radius);
    void addFluidStream(const glm::vec3& origin, const glm::vec3& direction, float rate);
    void addFluidStream(const glm::vec3& origin, const glm::vec3& direction); // Overload without rate
    void addFluidVolume(const glm::vec3& minPos, const glm::vec3& maxPos);
    
    
    // Getters for external systems
    WaterSurface* getWaterSurface() const { return waterSurface_.get(); }
    SPHComputeSystem* getSPHComputeSystem() const { return sphComputeSystem_.get(); }
    
    // State queries
    bool isRegularWaterActive() const { return currentType_ == SimulationType::REGULAR_WATER; }
    bool isSPHComputeActive() const { return currentType_ == SimulationType::SPH_COMPUTE; }
    
    // Configuration
    void setWaterHeight(float height) { waterHeight_ = height; }
    float getWaterHeight() const { return waterHeight_; }

private:
    const Config& config_;
    SimulationType currentType_;
    
    // Water simulations
    std::unique_ptr<WaterSurface> waterSurface_;
    std::unique_ptr<SPHComputeSystem> sphComputeSystem_;
    
    // State
    float waterHeight_;
    bool initialized_;
    
    // Private methods
    void initializeRegularWater();
    void initializeSPHCompute();
    void cleanupRegularWater();
    void cleanupSPHCompute();
};

} // namespace WaterSim
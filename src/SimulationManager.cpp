#include "SimulationManager.h"
#include <iostream>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace WaterSim {

SimulationManager::SimulationManager(const Config& config)
    : config_(config)
    , currentType_(SimulationType::NONE)
    , waterHeight_(0.0f)
    , initialized_(false)
{
}

SimulationManager::~SimulationManager() {
    cleanup();
}

void SimulationManager::setSimulationType(SimulationType type) {
    if (currentType_ == type) {
        return; // Already using this type
    }
    
    // Cleanup current simulation
    cleanup();
    
    // Set new type
    currentType_ = type;
    
    // Initialize new simulation
    if (currentType_ != SimulationType::NONE) {
        initialize();
    }
}

void SimulationManager::initialize() {
    if (initialized_) {
        cleanup();
    }
    
    switch (currentType_) {
        case SimulationType::REGULAR_WATER:
            initializeRegularWater();
            break;
        case SimulationType::SPH_COMPUTE:
            initializeSPHCompute();
            break;
        case SimulationType::NONE:
        default:
            break;
    }
    
    initialized_ = true;
}

void SimulationManager::cleanup() {
    if (!initialized_) {
        return;
    }
    
    switch (currentType_) {
        case SimulationType::REGULAR_WATER:
            cleanupRegularWater();
            break;
        case SimulationType::SPH_COMPUTE:
            cleanupSPHCompute();
            break;
        case SimulationType::NONE:
        default:
            break;
    }
    
    initialized_ = false;
}

void SimulationManager::update(float deltaTime) {
    if (!initialized_) {
        return;
    }
    
    switch (currentType_) {
        case SimulationType::REGULAR_WATER:
            if (waterSurface_) {
                waterSurface_->update(deltaTime);
            }
            break;
        case SimulationType::SPH_COMPUTE:
            if (sphComputeSystem_) {
                sphComputeSystem_->update(deltaTime);
            }
            break;
        case SimulationType::NONE:
        default:
            break;
    }
}

void SimulationManager::render(const glm::mat4& view, const glm::mat4& projection, 
                              unsigned int waterShader, bool rayTracingEnabled) {
    if (!initialized_) {
        return;
    }
    
    switch (currentType_) {
        case SimulationType::REGULAR_WATER:
            if (waterSurface_) {
                glUseProgram(waterShader);
                
                // Set transformation matrices
                glUniformMatrix4fv(glGetUniformLocation(waterShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
                glUniformMatrix4fv(glGetUniformLocation(waterShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
                
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, waterHeight_, 0.0f));
                glUniformMatrix4fv(glGetUniformLocation(waterShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                
                // Render water surface
                waterSurface_->render(waterShader);
            }
            break;
        case SimulationType::SPH_COMPUTE:
            if (sphComputeSystem_) {
                sphComputeSystem_->render(view, projection);
            }
            break;
        case SimulationType::NONE:
        default:
            break;
    }
}

void SimulationManager::addRipple(const glm::vec3& position, float magnitude) {
    if (currentType_ == SimulationType::REGULAR_WATER && waterSurface_) {
        waterSurface_->addRipple(position, magnitude);
    }
}

void SimulationManager::createSplash(const glm::vec3& position, float magnitude) {
    if (currentType_ == SimulationType::REGULAR_WATER && waterSurface_) {
        waterSurface_->createSplash(position, magnitude);
    }
}

void SimulationManager::addDirectionalRipple(const glm::vec3& position, const glm::vec2& direction, float magnitude) {
    if (currentType_ == SimulationType::REGULAR_WATER && waterSurface_) {
        waterSurface_->addDirectionalRipple(position, direction, magnitude);
    }
}

void SimulationManager::addWaterFlowImpulse(const glm::vec3& position, const glm::vec2& impulse, float radius) {
    if (currentType_ == SimulationType::REGULAR_WATER && waterSurface_) {
        waterSurface_->addImpulse(position, impulse, radius);
    }
}


void SimulationManager::initializeRegularWater() {
    std::cout << "Initializing Regular Water Simulation..." << std::endl;
    
    waterSurface_ = std::make_unique<WaterSurface>(100, 10.0f);
    waterSurface_->initialize();
    waterSurface_->setColor(glm::vec3(0.05f, 0.3f, 0.5f)); // Deep blue color
    waterSurface_->setTransparency(0.9f); // High transparency
    waterSurface_->clearWaves(); // Start with no waves
    
    // Add gentle default waves
    WaterSurface::WaveParam gentleWave;
    gentleWave.direction = glm::normalize(glm::vec2(1.0f, 0.7f));
    gentleWave.amplitude = 0.06f;
    gentleWave.wavelength = 12.0f;
    gentleWave.speed = 0.6f;
    gentleWave.steepness = 0.15f;
    waterSurface_->addWave(gentleWave);
    
    // Add second crossing wave
    WaterSurface::WaveParam crossWave;
    crossWave.direction = glm::normalize(glm::vec2(-0.6f, 1.0f));
    crossWave.amplitude = 0.03f;
    crossWave.wavelength = 8.0f;
    crossWave.speed = 0.8f;
    crossWave.steepness = 0.1f;
    waterSurface_->addWave(crossWave);
    
    std::cout << "Regular Water Simulation initialized successfully!" << std::endl;
}


void SimulationManager::cleanupRegularWater() {
    if (waterSurface_) {
        std::cout << "Cleaning up Regular Water Simulation..." << std::endl;
        waterSurface_.reset();
    }
}

void SimulationManager::applyImpulse(const glm::vec3& position, const glm::vec3& impulse, float radius) {
    if (currentType_ == SimulationType::SPH_COMPUTE && sphComputeSystem_) {
        // SPH Compute system doesn't have impulse API yet, could be added
    }
}

void SimulationManager::addFluidStream(const glm::vec3& origin, const glm::vec3& direction, float rate) {
    // Not implemented for SPH Compute yet
}

void SimulationManager::addFluidStream(const glm::vec3& origin, const glm::vec3& direction) {
    // Not implemented for SPH Compute yet
}

void SimulationManager::addFluidVolume(const glm::vec3& minPos, const glm::vec3& maxPos) {
    // Not implemented for SPH Compute yet
}


void SimulationManager::initializeSPHCompute() {
    std::cout << "Initializing SPH Compute Simulation" << std::endl;
    
    sphComputeSystem_ = std::make_unique<SPHComputeSystem>();
    
    // Initialize with box bounds matching the glass container
    // The glass container has dimensions 10x10x10, centered at origin
    // So bounds are -5 to 5 in each direction
    glm::vec3 boxMin(-5.0f, -5.0f, -5.0f);
    glm::vec3 boxMax(5.0f, 5.0f, 5.0f);
    
    // Initialize with up to 100k particles
    sphComputeSystem_->initialize(100000, boxMin, boxMax);
    
    std::cout << "SPH Compute Simulation initialized successfully!" << std::endl;
    std::cout << "Initial particles: " << sphComputeSystem_->getParticleCount() << std::endl;
}

void SimulationManager::cleanupSPHCompute() {
    if (sphComputeSystem_) {
        std::cout << "Cleaning up SPH Compute Simulation..." << std::endl;
        sphComputeSystem_.reset();
    }
}

} // namespace WaterSim
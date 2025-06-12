#pragma once

#include <glm/glm.hpp>
#include <string>

namespace WaterSim {

struct Config {
    // Display settings
    struct Display {
        unsigned int width = 1280;
        unsigned int height = 720;
        bool vsync = false;
        std::string title = "Water Simulation";
    } display;
    
    // Physics settings
    struct Physics {
        float gravity = 9.8f;
        float waterResistance = 2.0f;
        float bounceDamping = 0.3f;
        float floorLevel = -5.0f;
        float sphereRadius = 0.5f;
        float maxRippleMagnitude = 0.5f;
        float rippleChargeRate = 0.5f;
    } physics;
    
    // Water settings
    struct Water {
        float initialHeight = 0.0f;
        glm::vec3 color{0.05f, 0.3f, 0.5f};
        float transparency = 0.9f;
        int surfaceResolution = 100;
        float surfaceSize = 10.0f;
    } water;
    
    // Camera settings
    struct Camera {
        glm::vec3 startPosition{0.0f, 5.0f, 15.0f};
        float movementSpeed = 2.5f;
        float mouseSensitivity = 0.1f;
        float fov = 45.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
    } camera;
    
    // Texture settings
    struct Textures {
        int causticSize = 512;
        int tileSize = 512;
        int tilesPerTexture = 16;
    } textures;
    
    // SPH settings - Physically accurate water simulation for RTX
    struct SPH {
        int maxParticles = 50000;  // Dense particle simulation
        
        // Calculated SPH parameters for realistic water behavior
        float particleRadius = 0.05f;  // Small particles for smooth water surface
        float smoothingRadius = 0.15f;  // h = 3 * particleRadius (SPH standard)
        float restDensity = 1000.0f;  // Real water density (kg/m³)
        
        // Pressure calculation: cs² = K/ρ₀, where cs = 10 * vmax
        // For water: K ≈ 2200 MPa, but scaled for stability
        float gasConstant = 2000.0f;  // Tait equation stiffness
        float gamma = 7.0f;  // Tait equation exponent for water
        
        // Viscosity: μ = 0.001 Pa·s for water, scaled for particle simulation
        float viscosity = 0.05f;  // Kinematic viscosity for smooth flow
        
        float surfaceTension = 0.0728f;  // Real water surface tension (N/m)
        
        // Mass per particle: m = ρ₀ * V, where V = (4/3)πr³
        float particleMass = 0.000524f;  // Calculated: 1000 * (4/3) * π * (0.05)³
        
        // Time step: Δt ≤ 0.4 * sqrt(m/k) for stability
        float timeStep = 0.0001f;  // Small time step for stability
        
        // Spatial grid parameters
        int gridResolution = 128;  // High-res grid for neighbor search
        float particleSpacing = 0.1f;  // 2 * particleRadius for close packing
        
        // GPU optimization
        bool useGPUAcceleration = true;
        bool useCUDA = true;
        bool useOpenCL = true;
        bool useDirectCompute = true;
        bool enableSurfaceReconstruction = true;
        int workGroupSize = 256;
        int neighborLimit = 64;  // More neighbors for smooth interactions
        
        // Additional SPH parameters
        float boundaryDamping = 0.5f;  // Energy loss at boundaries
        float velocityLimit = 20.0f;   // Maximum particle velocity (m/s)
        float pressureLimit = 50000.0f; // Maximum pressure (Pa)
    } sph;
    
    // Debug settings
    struct Debug {
        bool showFPS = true;
        bool showWireframe = false;
        bool enableLogging = false;
        bool showSPHDebug = false;
        bool showSpatialGrid = false;
    } debug;
};

} // namespace WaterSim
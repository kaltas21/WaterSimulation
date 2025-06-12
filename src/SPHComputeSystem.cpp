#include "SPHComputeSystem.h"
#include "InitShader.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr

namespace WaterSim {

SPHComputeSystem::SPHComputeSystem()
    : numParticles_(0)
    , maxParticles_(0)
    , currentBuffer_(0)
    , colorMode_(COLOR_NORMAL)
    , useFilteredViscosity_(true)
    , curvatureFlowIterations_(50)
    , gravity_(0.0f, -9.81f, 0.0f)
    , spherePosition_(0.0f)
    , sphereImpulse_(0.0f)
    , sphereRadius_(0.0f)
    , sphereActive_(false)
    , renderContainer_(false)
    , accumulatedTime_(0.0f)
    , finalSmoothedBuffer_(0)
    , particleVAO_(0)
    , billboardVAO_(0)
    , gridTexture_(0)
    , billboardIndexBuffer_(0)
    , simStep1Program_(0)
    , simStep2Program_(0)
    , simStep3Program_(0)
    , simStep4Program_(0)
    , simStep5Program_(0)
    , simStep6Program_(0)
    , renderProgram_(0)
    , depthProgram_(0)
    , smoothProgram_(0)
    , finalProgram_(0)
    , depthFBO_(0)
    , depthTexture_(0)
    , containerVAO_(0)
    , containerVBO_(0)
    , containerEBO_(0)
    , containerShader_(0)
    , windowWidth_(1280)
    , windowHeight_(720)
{
    std::fill(std::begin(particleBuffers_), std::end(particleBuffers_), 0);
    std::fill(std::begin(smoothFBO_), std::end(smoothFBO_), 0);
    std::fill(std::begin(smoothTexture_), std::end(smoothTexture_), 0);
}

SPHComputeSystem::~SPHComputeSystem() {
    // Clean up OpenGL resources
    if (particleBuffers_[0]) glDeleteBuffers(2, particleBuffers_);
    if (particleVAO_) glDeleteVertexArrays(1, &particleVAO_);
    if (billboardVAO_) glDeleteVertexArrays(1, &billboardVAO_);
    if (gridTexture_) glDeleteTextures(1, &gridTexture_);
    if (billboardIndexBuffer_) glDeleteBuffers(1, &billboardIndexBuffer_);
    if (counterBuffer_) glDeleteBuffers(1, &counterBuffer_);
    if (velocityTexture_) glDeleteTextures(1, &velocityTexture_);
    
    if (simStep1Program_) glDeleteProgram(simStep1Program_);
    if (simStep2Program_) glDeleteProgram(simStep2Program_);
    if (simStep3Program_) glDeleteProgram(simStep3Program_);
    if (simStep4Program_) glDeleteProgram(simStep4Program_);
    if (simStep5Program_) glDeleteProgram(simStep5Program_);
    if (simStep6Program_) glDeleteProgram(simStep6Program_);
    if (renderProgram_) glDeleteProgram(renderProgram_);
    if (depthProgram_) glDeleteProgram(depthProgram_);
    if (smoothProgram_) glDeleteProgram(smoothProgram_);
    if (finalProgram_) glDeleteProgram(finalProgram_);
    
    if (depthFBO_) glDeleteFramebuffers(1, &depthFBO_);
    if (depthTexture_) glDeleteTextures(1, &depthTexture_);
    if (smoothFBO_[0]) glDeleteFramebuffers(2, smoothFBO_);
    if (smoothTexture_[0]) glDeleteTextures(2, smoothTexture_);
    
    if (containerVAO_) glDeleteVertexArrays(1, &containerVAO_);
    if (containerVBO_) glDeleteBuffers(1, &containerVBO_);
    if (containerEBO_) glDeleteBuffers(1, &containerEBO_);
    if (containerShader_) glDeleteProgram(containerShader_);
}

bool SPHComputeSystem::initialize(uint32_t numParticles, const glm::vec3& boxMin, const glm::vec3& boxMax) {
    // Ensure particle count is aligned for compute shaders
    uint32_t minParticles = std::max(numParticles, 50000u); // Start with 50k for testing
    maxParticles_ = ((minParticles + 511) / 512) * 512; 
    boxMin_ = boxMin;
    boxMax_ = boxMax;
    
    // Calculate grid like  but using our box bounds
    gridSize_ = boxMax - boxMin;
    gridCellSize_ = SPHConstants::CELL_SIZE;
    gridRes_ = glm::ivec3((gridSize_ / gridCellSize_) + 1.0f);
    gridOrigin_ = boxMin;
    gridDim_ = glm::uvec3(gridRes_);
    
    // Initialize OpenGL resources
    initializeGrid();
    createBuffers();
    loadShaders();
    createContainerGeometry();
    
    // Initialize with some particles
    reset();
    
    // Create framebuffers for screen-space fluid rendering
    createFramebuffers();
    
    return true;
}

void SPHComputeSystem::createFramebuffers() {
    // Create depth framebuffer with color attachment for now (depth-only rendering can be tricky)
    glGenFramebuffers(1, &depthFBO_);
    glGenTextures(1, &depthTexture_);
    
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO_);
    
    // Create color texture for debugging
    GLuint colorTexture;
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, windowWidth_, windowHeight_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    
    // Depth texture
    glBindTexture(GL_TEXTURE_2D, depthTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, windowWidth_, windowHeight_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture_, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Depth framebuffer is not complete!" << std::endl;
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        std::cerr << "Framebuffer status: " << status << std::endl;
    }
    
    // Create smoothing framebuffers (ping-pong)
    glGenFramebuffers(2, smoothFBO_);
    glGenTextures(2, smoothTexture_);
    
    for (int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, smoothFBO_[i]);
        
        glBindTexture(GL_TEXTURE_2D, smoothTexture_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, windowWidth_, windowHeight_, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smoothTexture_[i], 0);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR: Smooth framebuffer " << i << " is not complete!" << std::endl;
        }
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SPHComputeSystem::initializeGrid() {
    // Create 3D texture for spatial grid (stores head of linked list)
    glGenTextures(1, &gridTexture_);
    glBindTexture(GL_TEXTURE_3D, gridTexture_);
    // GL_R32UI will store uint32_t, which will be the index of the first particle in that cell
    // or SPHConstants::INVALID_INDEX if the cell is empty.
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, gridDim_.x, gridDim_.y, gridDim_.z, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    std::cout << "Grid Texture initialized (ID: " << gridTexture_ << ") with dimensions: " 
              << gridDim_.x << "x" << gridDim_.y << "x" << gridDim_.z << std::endl;
}

void SPHComputeSystem::createBuffers() {
    // Create particle buffers using modern OpenGL
    glCreateBuffers(2, particleBuffers_);
    
    size_t bufferSize = maxParticles_ * sizeof(SPHParticleCompute);
    for (int i = 0; i < 2; i++) {
        glNamedBufferStorage(particleBuffers_[i], bufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    }

    // Create counter buffer for grid operations
    glCreateBuffers(1, &counterBuffer_);
    glNamedBufferStorage(counterBuffer_, sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);
    
    // Create velocity field texture for step 4
    glGenTextures(1, &velocityTexture_);
    glBindTexture(GL_TEXTURE_3D, velocityTexture_);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, gridDim_.x, gridDim_.y, gridDim_.z, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    // Create billboard index buffer
    {
        uint32_t billboardIndexCount = 6;
        uint32_t billboardVertexCount = 4;
        uint32_t billboardIndices[] = { 0, 1, 2, 2, 1, 3 };
        
        std::vector<uint32_t> indices(billboardIndexCount * maxParticles_);
        
        for (uint32_t i = 0; i < indices.size(); i++) {
            uint32_t particleOffset = i / billboardIndexCount;
            uint32_t particleIndexOffset = i % billboardIndexCount;
            indices[i] = billboardIndices[particleIndexOffset] + particleOffset * billboardVertexCount;
        }
        
        glCreateBuffers(1, &billboardIndexBuffer_);
        glNamedBufferStorage(billboardIndexBuffer_, indices.size() * sizeof(uint32_t), indices.data(), 0);
    }

    glGenVertexArrays(1, &particleVAO_);
    glBindVertexArray(particleVAO_);
    glBindVertexArray(0);
    
    // Create billboard VAO like 
    glGenVertexArrays(1, &billboardVAO_);
    glBindVertexArray(billboardVAO_);
    
    // Bind the billboard index buffe
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboardIndexBuffer_);
    
    
    glBindVertexArray(0);
}

void SPHComputeSystem::loadShaders() {
    // Load all 6 simulation shaders
    simStep1Program_ = InitComputeShader("shaders/sph_step1.cs");
    if (!simStep1Program_) {
        std::cerr << "ERROR: Failed to load SPH step 1 shader!" << std::endl;
    } else {
        std::cout << "SPH step 1 shader loaded successfully (ID: " << simStep1Program_ << ")" << std::endl;
    }
    
    simStep2Program_ = InitComputeShader("shaders/sph_step2.cs");
    if (!simStep2Program_) {
        std::cerr << "ERROR: Failed to load SPH step 2 shader!" << std::endl;
    } else {
        std::cout << "SPH step 2 shader loaded successfully (ID: " << simStep2Program_ << ")" << std::endl;
    }
    
    simStep3Program_ = InitComputeShader("shaders/sph_step3.cs");
    if (!simStep3Program_) {
        std::cerr << "ERROR: Failed to load SPH step 3 shader!" << std::endl;
    } else {
        std::cout << "SPH step 3 shader loaded successfully (ID: " << simStep3Program_ << ")" << std::endl;
    }
    
    simStep4Program_ = InitComputeShader("shaders/sph_step4.cs");
    if (!simStep4Program_) {
        std::cerr << "ERROR: Failed to load SPH step 4 shader!" << std::endl;
    } else {
        std::cout << "SPH step 4 shader loaded successfully (ID: " << simStep4Program_ << ")" << std::endl;
    }
    
    simStep5Program_ = InitComputeShader("shaders/sph_step5.cs");
    if (!simStep5Program_) {
        std::cerr << "ERROR: Failed to load SPH step 5 shader!" << std::endl;
    } else {
        std::cout << "SPH step 5 shader loaded successfully (ID: " << simStep5Program_ << ")" << std::endl;
    }
    
    simStep6Program_ = InitComputeShader("shaders/sph_step6.cs");
    if (!simStep6Program_) {
        std::cerr << "ERROR: Failed to load SPH step 6 shader!" << std::endl;
    } else {
        std::cout << "SPH step 6 shader loaded successfully (ID: " << simStep6Program_ << ")" << std::endl;
    }
    
    // Load rendering shaders
    renderProgram_ = InitShader("shaders/sph_render.vs", "shaders/sph_render.fs");
    if (!renderProgram_) {
        std::cerr << "ERROR: Failed to load SPH rendering shaders!" << std::endl;
    } else {
        std::cout << "SPH rendering shaders loaded successfully (ID: " << renderProgram_ << ")" << std::endl;
    }
    
    // Load screen-space fluid rendering shaders
    depthProgram_ = InitShader("shaders/sph_depth.vs", "shaders/sph_depth.fs");
    if (!depthProgram_) {
        std::cerr << "ERROR: Failed to load SPH depth shaders!" << std::endl;
    } else {
        std::cout << "SPH depth shaders loaded successfully (ID: " << depthProgram_ << ")" << std::endl;
    }
    
    smoothProgram_ = InitShader("shaders/sph_smooth.vs", "shaders/sph_smooth.fs");
    if (!smoothProgram_) {
        std::cerr << "ERROR: Failed to load SPH smooth shaders!" << std::endl;
    } else {
        std::cout << "SPH smooth shaders loaded successfully (ID: " << smoothProgram_ << ")" << std::endl;
    }
    
    finalProgram_ = InitShader("shaders/sph_final.vs", "shaders/sph_final.fs");
    if (!finalProgram_) {
        std::cerr << "ERROR: Failed to load SPH final shaders!" << std::endl;
    } else {
        std::cout << "SPH final shaders loaded successfully (ID: " << finalProgram_ << ")" << std::endl;
    }
    
    // Load container shader (reuse glass shader)
    containerShader_ = InitShader("shaders/glass.vs", "shaders/glass.fs");
    if (!containerShader_) {
        std::cerr << "ERROR: Failed to load container shader!" << std::endl;
    } else {
        std::cout << "Container shader loaded successfully (ID: " << containerShader_ << ")" << std::endl;
    }
}

void SPHComputeSystem::createContainerGeometry() {
    // Create a wireframe box for the container
    std::vector<glm::vec3> vertices = {
        // Bottom face
        glm::vec3(boxMin_.x, boxMin_.y, boxMin_.z),
        glm::vec3(boxMax_.x, boxMin_.y, boxMin_.z),
        glm::vec3(boxMax_.x, boxMin_.y, boxMax_.z),
        glm::vec3(boxMin_.x, boxMin_.y, boxMax_.z),
        // Top face
        glm::vec3(boxMin_.x, boxMax_.y, boxMin_.z),
        glm::vec3(boxMax_.x, boxMax_.y, boxMin_.z),
        glm::vec3(boxMax_.x, boxMax_.y, boxMax_.z),
        glm::vec3(boxMin_.x, boxMax_.y, boxMax_.z)
    };
    
    std::vector<unsigned int> indices = {
        // Bottom face
        0, 1, 2, 2, 3, 0,
        // Top face
        4, 5, 6, 6, 7, 4,
        // Sides
        0, 1, 5, 5, 4, 0,
        1, 2, 6, 6, 5, 1,
        2, 3, 7, 7, 6, 2,
        3, 0, 4, 4, 7, 3
    };
    
    glGenVertexArrays(1, &containerVAO_);
    glGenBuffers(1, &containerVBO_);
    glGenBuffers(1, &containerEBO_);
    
    glBindVertexArray(containerVAO_);
    
    glBindBuffer(GL_ARRAY_BUFFER, containerVBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, containerEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void SPHComputeSystem::reset() {
    numParticles_ = 0;
    
    // Initialize particles in a dam break scenario inside the container
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> velocities;
    
    // Create a dam-break fluid block inside our box container
    float spacing = SPHConstants::PARTICLE_RADIUS * 2.0f;  // Standard SPH spacing
    
    // Start from 25% into the box, extend to 75% in X and Z, but only 50% in Y (half-height)
    glm::vec3 fluidMin = gridOrigin_ + gridSize_ * 0.25f;
    glm::vec3 fluidMax = gridOrigin_ + gridSize_ * 0.75f;
    fluidMax.y = gridOrigin_.y + gridSize_.y * 0.5f; // Half height for dam break
    
    // Ensure particles stay inside box bounds with some margin
    float margin = SPHConstants::PARTICLE_RADIUS;
    fluidMin = glm::max(fluidMin, boxMin_ + glm::vec3(margin));
    fluidMax = glm::min(fluidMax, boxMax_ - glm::vec3(margin));
    
    std::cout << "Creating SPH fluid dam break simulation:" << std::endl;
    std::cout << "Container bounds: " << glm::to_string(boxMin_) << " to " << glm::to_string(boxMax_) << std::endl;
    std::cout << "Fluid block: " << glm::to_string(fluidMin) << " to " << glm::to_string(fluidMax) << std::endl;
    std::cout << "Particle spacing: " << spacing << std::endl;
    
    // For best results, initialize with similar aspect ratio container
    glm::vec3 containerSize = boxMax_ - boxMin_;
    std::cout << "Container size: " << glm::to_string(containerSize) << std::endl;
    std::cout << "Container volume: " << (containerSize.x * containerSize.y * containerSize.z) << " cubic units" << std::endl;
    
    int particleCount = 0;
    for (float x = fluidMin.x; x <= fluidMax.x; x += spacing) {
        for (float y = fluidMin.y; y <= fluidMax.y; y += spacing) {
            for (float z = fluidMin.z; z <= fluidMax.z; z += spacing) {
                positions.push_back(glm::vec3(x, y, z));
                velocities.push_back(glm::vec3(0.0f)); // Start at rest
                particleCount++;
                
                // O(n)
                if (particleCount >= maxParticles_) goto done_creating;
            }
        }
    }
    
    done_creating:
    std::cout << "Created " << positions.size() << " fluid particles for dam break" << std::endl;
    addParticles(positions, velocities);
    std::cout << "SPH system initialized with " << numParticles_ << " particles" << std::endl;
}

void SPHComputeSystem::applyImpulse(const glm::vec3& position, const glm::vec3& impulse, float radius) {
    spherePosition_ = position;
    sphereImpulse_ = impulse;
    sphereRadius_ = radius;
    sphereActive_ = true;
    
    std::cout << "SPH: Applied impulse at (" << position.x << ", " << position.y << ", " << position.z 
              << ") with magnitude " << glm::length(impulse) << " and radius " << radius << std::endl;
}

void SPHComputeSystem::addParticles(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& velocities) {
    if (positions.size() != velocities.size()) {
        std::cerr << "Position and velocity arrays must have same size" << std::endl;
        return;
    }
    
    // Create local copies that we can modify if needed
    std::vector<glm::vec3> finalPositions = positions;
    std::vector<glm::vec3> finalVelocities = velocities;
    
    if (numParticles_ + finalPositions.size() > maxParticles_) {
        std::cerr << "Too many particles! Requested: " << (numParticles_ + finalPositions.size()) 
                  << ", Max: " << maxParticles_ << std::endl;
        // Add as many particles as we can
        size_t maxToAdd = maxParticles_ - numParticles_;
        if (maxToAdd == 0) return;
        
        std::cout << "Adding only " << maxToAdd << " particles instead" << std::endl;
        finalPositions.resize(maxToAdd);
        finalVelocities.resize(maxToAdd);
    }
    
    // Create particle data
    std::vector<SPHParticleCompute> particles(finalPositions.size());
    for (size_t i = 0; i < finalPositions.size(); i++) {
        particles[i].position = finalPositions[i];
        particles[i].velocity = finalVelocities[i];
        particles[i].density = SPHConstants::REST_DENSITY;
        particles[i].pressure = 0.0f;
    }
    
    // Upload to GPU using modern OpenGL
    glNamedBufferSubData(particleBuffers_[currentBuffer_], numParticles_ * sizeof(SPHParticleCompute),
                         particles.size() * sizeof(SPHParticleCompute), particles.data());
    
    numParticles_ += static_cast<uint32_t>(finalPositions.size());
    
    std::cout << "Added " << finalPositions.size() << " particles. Total: " << numParticles_ << std::endl;
    
    // Verify particles were uploaded correctly
    if (numParticles_ > 0) {
        std::cout << "SUCCESS: Particles added to buffer" << std::endl;
    } else {
        std::cerr << "ERROR: Failed to add particles!" << std::endl;
    }
}

void SPHComputeSystem::update(float deltaTime) {
    if (numParticles_ == 0) return;
    
    // DEBUG: Print update info and sample particle positions
    static int updateCount = 0;
    if (updateCount++ % 60 == 0) {
        std::cout << "SPH Update: " << numParticles_ << " particles, dt=" << deltaTime << std::endl;
        
        // Sample first few particle positions for debugging
        if (numParticles_ > 0) {
            SPHParticleCompute* particles = static_cast<SPHParticleCompute*>(
                glMapNamedBufferRange(particleBuffers_[currentBuffer_], 0, 
                                    sizeof(SPHParticleCompute) * std::min(5u, numParticles_), 
                                    GL_MAP_READ_BIT));
            if (particles) {
                std::cout << "Sample particle positions:" << std::endl;
                for (uint32_t i = 0; i < std::min(5u, numParticles_); i++) {
                    std::cout << "  Particle " << i << ": pos(" << particles[i].position.x 
                              << ", " << particles[i].position.y << ", " << particles[i].position.z 
                              << ") vel(" << glm::length(particles[i].velocity) << ")" << std::endl;
                }
                glUnmapNamedBuffer(particleBuffers_[currentBuffer_]);
            }
        }
    }
    
    // Simple fixed timestep update
    accumulatedTime_ += deltaTime;
    
    while (accumulatedTime_ >= SPHConstants::DT) {
        // Clear grid
        uint32_t clearValue = 0;
        glClearTexImage(gridTexture_, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clearValue);
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

        // Run full SPH simulation pipeline with O(n) spatial hashing
        runSimulationPass(1); // Step 1: Position integration and grid population
        runSimulationPass(2); // Step 2: Grid offset calculation  
        runSimulationPass(3); // Step 3: Particle reordering
        runSimulationPass(4); // Step 4: Velocity field calculation (optional, for filtered viscosity)
        runSimulationPass(5); // Step 5: Density and pressure calculation (O(n) with spatial hashing)
        runSimulationPass(6); // Step 6: Force calculation (O(n) with spatial hashing)
        
        accumulatedTime_ -= SPHConstants::DT;
    }
}

void SPHComputeSystem::runSimulationPass(int pass) {
    glm::vec3 invCellSize = glm::vec3(gridRes_) * (1.0f - 0.001f) / gridSize_;
    
    switch (pass) {
        case 1: // Step 1: Position integration and grid population
            if (simStep1Program_) {
                glUseProgram(simStep1Program_);
                
                glUniform1f(glGetUniformLocation(simStep1Program_, "uDT"), SPHConstants::DT);
                glUniform3fv(glGetUniformLocation(simStep1Program_, "uGravity"), 1, &gravity_[0]);
                glUniform3fv(glGetUniformLocation(simStep1Program_, "uGridOrigin"), 1, &gridOrigin_[0]);
                glUniform3fv(glGetUniformLocation(simStep1Program_, "uGridSize"), 1, &gridSize_[0]);
                glUniform3fv(glGetUniformLocation(simStep1Program_, "uInvCellSize"), 1, &invCellSize[0]);
                
                // Sphere collision uniforms
                glUniform3fv(glGetUniformLocation(simStep1Program_, "uSpherePosition"), 1, &spherePosition_[0]);
                glUniform3fv(glGetUniformLocation(simStep1Program_, "uSphereImpulse"), 1, &sphereImpulse_[0]);
                glUniform1f(glGetUniformLocation(simStep1Program_, "uSphereRadius"), sphereRadius_);
                glUniform1i(glGetUniformLocation(simStep1Program_, "uSphereActive"), sphereActive_ ? 1 : 0);
                
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers_[currentBuffer_]);
                glBindImageTexture(0, gridTexture_, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
                
                uint32_t workGroups = (numParticles_ + 31) / 32;
                glDispatchCompute(workGroups, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                
                // Reset sphere impulse after applying it
                if (sphereActive_) {
                    sphereActive_ = false;
                    sphereImpulse_ = glm::vec3(0.0f);
                }
            }
            break;
            
        case 2: // Step 2: Grid offset calculation
            if (simStep2Program_) {
                glUseProgram(simStep2Program_);
                
                // Clear counter buffer
                uint32_t zero = 0;
                glNamedBufferSubData(counterBuffer_, 0, sizeof(uint32_t), &zero);
                
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, counterBuffer_);
                glBindImageTexture(0, gridTexture_, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
                
                uint32_t workGroupsX = (gridDim_.x + 3) / 4;
                uint32_t workGroupsY = (gridDim_.y + 3) / 4;
                uint32_t workGroupsZ = (gridDim_.z + 3) / 4;
                glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            }
            break;
            
        case 3: // Step 3: Particle reordering
            if (simStep3Program_) {
                glUseProgram(simStep3Program_);
                
                glUniform3fv(glGetUniformLocation(simStep3Program_, "uInvCellSize"), 1, &invCellSize[0]);
                glUniform3fv(glGetUniformLocation(simStep3Program_, "uGridOrigin"), 1, &gridOrigin_[0]);
                
                // Bind input buffer (current) and output buffer (opposite)
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers_[currentBuffer_]);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particleBuffers_[1 - currentBuffer_]);
                glBindImageTexture(0, gridTexture_, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
                
                uint32_t workGroups = (numParticles_ + 31) / 32;
                glDispatchCompute(workGroups, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                
                // Swap buffers after reordering
                swapBuffers();
            }
            break;
            
        case 4: // Step 4: Velocity field calculation
            if (simStep4Program_) {
                glUseProgram(simStep4Program_);
                
                glUniform3fv(glGetUniformLocation(simStep4Program_, "uGridOrigin"), 1, &gridOrigin_[0]);
                glUniform3fv(glGetUniformLocation(simStep4Program_, "uGridSize"), 1, &gridSize_[0]);
                glUniform3iv(glGetUniformLocation(simStep4Program_, "uGridRes"), 1, &gridRes_[0]);
                
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers_[currentBuffer_]);
                glBindImageTexture(0, gridTexture_, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32UI);
                glBindImageTexture(1, velocityTexture_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                
                uint32_t workGroupsX = (gridDim_.x + 3) / 4;
                uint32_t workGroupsY = (gridDim_.y + 3) / 4;
                uint32_t workGroupsZ = (gridDim_.z + 3) / 4;
                glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            }
            break;
            
        case 5: // Step 5: Density and pressure calculation
            if (simStep5Program_) {
                glUseProgram(simStep5Program_);
                
                glUniform3fv(glGetUniformLocation(simStep5Program_, "uInvCellSize"), 1, &invCellSize[0]);
                glUniform3fv(glGetUniformLocation(simStep5Program_, "uGridOrigin"), 1, &gridOrigin_[0]);
                glUniform3iv(glGetUniformLocation(simStep5Program_, "uGridRes"), 1, &gridRes_[0]);
                
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers_[currentBuffer_]);
                glBindImageTexture(0, gridTexture_, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32UI);
                
                uint32_t workGroups = (numParticles_ + 63) / 64;
                glDispatchCompute(workGroups, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
            break;
            
        case 6: // Step 6: Force calculation
            if (simStep6Program_) {
                glUseProgram(simStep6Program_);
                
                glUniform1f(glGetUniformLocation(simStep6Program_, "uDT"), SPHConstants::DT);
                glUniform3fv(glGetUniformLocation(simStep6Program_, "uGravity"), 1, &gravity_[0]);
                glUniform3fv(glGetUniformLocation(simStep6Program_, "uInvCellSize"), 1, &invCellSize[0]);
                glUniform3fv(glGetUniformLocation(simStep6Program_, "uGridOrigin"), 1, &gridOrigin_[0]);
                glUniform3iv(glGetUniformLocation(simStep6Program_, "uGridRes"), 1, &gridRes_[0]);
                
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers_[currentBuffer_]);
                glBindImageTexture(0, gridTexture_, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32UI);
                
                // Bind velocity texture for filtered viscosity (optional)
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_3D, velocityTexture_);
                glUniform1i(glGetUniformLocation(simStep6Program_, "velocityField"), 0);
                
                uint32_t workGroups = (numParticles_ + 63) / 64;
                glDispatchCompute(workGroups, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
            break;
    }
}

void SPHComputeSystem::swapBuffers() {
    currentBuffer_ = 1 - currentBuffer_;
}

void SPHComputeSystem::render(const glm::mat4& view, const glm::mat4& projection) {
    if (numParticles_ == 0) return;
    
    // Don't bind framebuffer here - let the caller control which framebuffer is active
    // The main rendering loop handles framebuffer management
    
    // Render particles first, then container
    // This ensures particles are visible through the transparent container
    renderParticles(view, projection);
    
    // Then render the container if enabled (with transparency)
    if (renderContainer_) {
        renderGlassContainer(view, projection);
    }
}

void SPHComputeSystem::renderParticles(const glm::mat4& view, const glm::mat4& projection) {
    if (numParticles_ == 0) return;
    
    // Debug output (increased frequency for debugging)
    static int frameCount = 0;
    bool debugFrame = (frameCount++ % 30 == 0); // More frequent debugging
    if (debugFrame) {
        std::cout << "=== SPH Particle Rendering Debug ===" << std::endl;
        std::cout << "Rendering " << numParticles_ << " particles" << std::endl;
        std::cout << "Current buffer: " << currentBuffer_ << std::endl;
        std::cout << "Render program: " << renderProgram_ << std::endl;
        std::cout << "Billboard VAO: " << billboardVAO_ << std::endl;
        std::cout << "Billboard index buffer: " << billboardIndexBuffer_ << std::endl;
        std::cout << "Point radius: " << (SPHConstants::KERNEL_RADIUS * 2.0f) << std::endl;
        
        // Check current framebuffer
        GLint currentFBO;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
        std::cout << "Current framebuffer: " << currentFBO << std::endl;
        
        // Check uniform locations
        if (renderProgram_) {
            std::cout << "Uniform locations: uVP=" << glGetUniformLocation(renderProgram_, "uVP") 
                      << " uParticleCount=" << glGetUniformLocation(renderProgram_, "uParticleCount") << std::endl;
        }
    }
    
    // Temporarily use simple rendering to debug black particles
    static bool useSimpleRendering = true; // Use simple rendering for debugging
    
    if (debugFrame) {
        std::cout << "Using simple rendering: " << useSimpleRendering << std::endl;
    }
    
    if (useSimpleRendering) {
        renderParticlesAsPoints(view, projection);
    } else {
        // Screen-space fluid rendering pipeline (matching )
        renderScreenSpaceFluid(view, projection);
    }
}

void SPHComputeSystem::renderParticlesAsPoints(const glm::mat4& view, const glm::mat4& projection) {
    if (!renderProgram_ || numParticles_ == 0) {
        std::cout << "WARNING: Cannot render particles - program:" << renderProgram_ << " particles:" << numParticles_ << std::endl;
        return;
    }
    
    // Removed debug clear - issue was the post-processing clear
    
    // Enable depth testing and blending for proper rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(renderProgram_);
    
    // Set 
    glm::mat4 vp = projection * view;
    const float pointRadius = 0.5f; // Much larger fixed size for debugging
    
    // Set uniforms with error checking
    GLint vpLoc = glGetUniformLocation(renderProgram_, "uVP");
    GLint viewLoc = glGetUniformLocation(renderProgram_, "uView");
    GLint projLoc = glGetUniformLocation(renderProgram_, "uProjection");
    GLint radiusLoc = glGetUniformLocation(renderProgram_, "uPointRadius");
    GLint countLoc = glGetUniformLocation(renderProgram_, "uParticleCount");
    GLint colorLoc = glGetUniformLocation(renderProgram_, "uColorMode");
    
    if (vpLoc != -1) glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &vp[0][0]);
    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    if (radiusLoc != -1) glUniform1f(radiusLoc, pointRadius);
    if (countLoc != -1) glUniform1ui(countLoc, numParticles_);
    if (colorLoc != -1) glUniform1i(colorLoc, static_cast<int>(colorMode_));
    
    // Grid parameters are optional for basic rendering
    GLint gridSizeLoc = glGetUniformLocation(renderProgram_, "uGridSize");
    GLint gridOriginLoc = glGetUniformLocation(renderProgram_, "uGridOrigin");
    GLint gridResLoc = glGetUniformLocation(renderProgram_, "uGridRes");
    if (gridSizeLoc != -1) glUniform3fv(gridSizeLoc, 1, &gridSize_[0]);
    if (gridOriginLoc != -1) glUniform3fv(gridOriginLoc, 1, &gridOrigin_[0]);
    if (gridResLoc != -1) glUniform3iv(gridResLoc, 1, &gridRes_[0]);
    
    // Bind particle buffer as SSBO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers_[currentBuffer_]);
    
    // Use billboard VAO like 
    glBindVertexArray(billboardVAO_);
    
    // DEBUG: Also try rendering as points
    static int testMode = 0;
    static int testFrameCount = 0;
    if (testFrameCount++ % 90 == 0) {
        testMode = (testMode + 1) % 3;
        std::cout << "Test mode: " << testMode << " (0=billboards, 1=points, 2=both)" << std::endl;
    }
    
    if (testMode == 0 || testMode == 2) {
        // Render as indexed billboards exactly like : 6 indices per particle (2 triangles per quad)
        uint32_t indexCount = 6 * numParticles_;
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    }
    
    if (testMode == 1 || testMode == 2) {
        // Try simple point rendering
        glPointSize(10.0f); // Large points for visibility
        glDrawArrays(GL_POINTS, 0, numParticles_);
    }
    
    // Check for errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        static int errorCount = 0;
        if (errorCount++ < 3) {
            std::cerr << "OpenGL error in billboard rendering: 0x" << std::hex << err << std::dec;
            if (err == GL_INVALID_OPERATION) std::cerr << " (GL_INVALID_OPERATION)";
            else if (err == GL_INVALID_VALUE) std::cerr << " (GL_INVALID_VALUE)";
            else if (err == GL_INVALID_ENUM) std::cerr << " (GL_INVALID_ENUM)";
            std::cerr << std::endl;
            
            // Additional debug info
            std::cerr << "IndexCount: " << (6 * numParticles_) << ", NumParticles: " << numParticles_ << std::endl;
            std::cerr << "VAO bound: " << billboardVAO_ << ", Buffer bound: " << particleBuffers_[currentBuffer_] << std::endl;
        }
    }
    
    glBindVertexArray(0);
}

void SPHComputeSystem::renderScreenSpaceFluid(const glm::mat4& view, const glm::mat4& projection) {
    // Step 1: Render particles as sphere impostors to depth buffer
    renderParticleDepth(view, projection);
    
    // Step 2: Apply curvature flow smoothing
    applyCurvatureFlowSmoothing();
    
    // Step 3: Final shading pass to screen
    renderFinalShading(view, projection);
}

void SPHComputeSystem::renderParticleDepth(const glm::mat4& view, const glm::mat4& projection) {
    if (!depthProgram_) return;
    
    // Bind depth framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO_);
    glViewport(0, 0, windowWidth_, windowHeight_);
    
    // Clear depth and color - use far depth value (1.0) for background
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // White background depth
    glClearDepth(1.0f); // Clear to far plane
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    
    // Use depth rendering shader
    glUseProgram(depthProgram_);
    
    // Set uniforms
    glm::mat4 mvp = projection * view;
    const float pointRadius = SPHConstants::KERNEL_RADIUS * 2.0f; // Larger for visibility
    
    static int debugCount = 0;
    if (debugCount++ % 60 == 0) {
        std::cout << "=== Depth Rendering Debug ===" << std::endl;
        std::cout << "Particles: " << numParticles_ << ", Point radius: " << pointRadius << std::endl;
        std::cout << "FBO: " << depthFBO_ << ", VAO: " << billboardVAO_ << std::endl;
    }
    
    glUniformMatrix4fv(glGetUniformLocation(depthProgram_, "uMVP"), 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(depthProgram_, "uView"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(depthProgram_, "uProjection"), 1, GL_FALSE, &projection[0][0]);
    glUniform1f(glGetUniformLocation(depthProgram_, "uPointRadius"), pointRadius);
    glUniform1ui(glGetUniformLocation(depthProgram_, "uNumParticles"), numParticles_);
    
    // Bind particle buffer as SSBO (same as main rendering)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers_[currentBuffer_]);
    
    // Use billboard VAO (same approach as main rendering)
    glBindVertexArray(billboardVAO_);
    
    // Render as indexed billboards like main rendering: 6 indices per particle
    uint32_t indexCount = 6 * numParticles_;
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    
    // Check for OpenGL errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR && debugCount <= 5) {
        std::cerr << "OpenGL error in depth rendering: " << err << std::endl;
    }
    
    glBindVertexArray(0);
}

void SPHComputeSystem::applyCurvatureFlowSmoothing() {
    if (!smoothProgram_) return;
    
    glUseProgram(smoothProgram_);
    
    // Disable depth testing for fullscreen passes
    glDisable(GL_DEPTH_TEST);
    
    // Set screen dimensions
    glUniform2i(glGetUniformLocation(smoothProgram_, "uScreenSize"), windowWidth_, windowHeight_);
    
    // Create a fullscreen quad VAO if we don't have one
    static GLuint fullscreenVAO = 0;
    if (fullscreenVAO == 0) {
        glGenVertexArrays(1, &fullscreenVAO);
        glBindVertexArray(fullscreenVAO);
        // No vertex buffer needed - generate vertices in shader
        glBindVertexArray(0);
    }
    
    // Ping-pong between two smoothing framebuffers
    GLuint inputTexture = depthTexture_;
    int outputBuffer = 0; // Start writing to buffer 0
    
    glBindVertexArray(fullscreenVAO);
    
    // Apply 50 iterations of curvature flow smoothing 
    for (int i = 0; i < curvatureFlowIterations_; ++i) {
        // Bind output framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, smoothFBO_[outputBuffer]);
        glClear(GL_COLOR_BUFFER_BIT); // Clear the target buffer

        // Bind input texture (from previous pass or initial depth texture)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        glUniform1i(glGetUniformLocation(smoothProgram_, "uDepthTexture"), 0);

        // Render fullscreen triangle (vertex shader generates vertices from gl_VertexID)
        glDrawArrays(GL_TRIANGLES, 0, 3); 

        // Update for next iteration: output texture becomes input, switch output buffer
        inputTexture = smoothTexture_[outputBuffer];
        outputBuffer = 1 - outputBuffer; // Ping-pong between 0 and 1
    }
    
    // Store the final result buffer index for the final shading pass
    finalSmoothedBuffer_ = 1 - outputBuffer; // The last written buffer
    
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void SPHComputeSystem::renderFinalShading(const glm::mat4& view, const glm::mat4& projection) {
    // Render final water surface to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth_, windowHeight_);
    
    // Enable blending for transparent water
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    if (finalProgram_) {
        glUseProgram(finalProgram_);
        
        // Bind the final smoothed texture (stored during smoothing pass)
        glActiveTexture(GL_TEXTURE0);
        GLuint finalTexture = smoothTexture_[finalSmoothedBuffer_];
        glBindTexture(GL_TEXTURE_2D, finalTexture);
        glUniform1i(glGetUniformLocation(finalProgram_, "uTexture"), 0);
        
        // Render fullscreen quad
        static GLuint fullscreenVAO = 0;
        if (fullscreenVAO == 0) {
            glGenVertexArrays(1, &fullscreenVAO);
        }
        glBindVertexArray(fullscreenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }
    
    // Restore state
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void SPHComputeSystem::renderGlassContainer(const glm::mat4& view, const glm::mat4& projection) {
    if (!containerShader_ || !renderContainer_) return;
    
    glUseProgram(containerShader_);
    
    // Set transformation matrices
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(containerShader_, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(containerShader_, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(containerShader_, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    // Set glass properties
    glUniform3f(glGetUniformLocation(containerShader_, "glassColor"), 0.9f, 0.95f, 1.0f);
    glUniform1f(glGetUniformLocation(containerShader_, "glassAlpha"), 0.2f);
    glUniform1f(glGetUniformLocation(containerShader_, "glassRefractionIndex"), 1.05f);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render container
    glBindVertexArray(containerVAO_);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void SPHComputeSystem::onWindowResize(int width, int height) {
    if (width == windowWidth_ && height == windowHeight_) return;
    
    windowWidth_ = width;
    windowHeight_ = height;
    
    // Recreate framebuffers with new size
    if (depthFBO_) {
        glDeleteFramebuffers(1, &depthFBO_);
        glDeleteTextures(1, &depthTexture_);
        glDeleteFramebuffers(2, smoothFBO_);
        glDeleteTextures(2, smoothTexture_);
    }
    
    createFramebuffers();
}

}

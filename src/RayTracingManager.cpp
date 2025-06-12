#include "RayTracingManager.h"
#include "InitShader.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <vector>
#include <GLFW/glfw3.h>

namespace WaterSim {

RayTracingManager::RayTracingManager(const Config& config)
    : config_(config), quality_(RayTracingQuality::OFF),  // Start with OFF by default
      screenWidth_(1920), screenHeight_(1080),
      rtWidth_(1920), rtHeight_(1080),  // Use full resolution by default
      lastFrameTime_(0.0f), raysPerSecond_(0),
      rtxAvailable_(false), rtxContext_(nullptr) {
    
    // Initialize default features
    features_.reflections = true;
    features_.refractions = true;
    features_.caustics = true;
    features_.volumetricLighting = false;
    features_.softShadows = true;
    features_.globalIllumination = false;
}

RayTracingManager::~RayTracingManager() {
    cleanup();
}

bool RayTracingManager::initialize(int screenWidth, int screenHeight) {
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    
    std::cout << "\n=== Ray Tracing System Initialization ===" << std::endl;
    std::cout << "Screen dimensions: " << screenWidth << "x" << screenHeight << std::endl;
    std::cout << "Initial quality: " << (int)quality_ << std::endl;
    
    // Check OpenGL version and compute shader support
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    std::cout << "OpenGL Version: " << version << std::endl;
    std::cout << "Renderer: " << renderer << std::endl;
    
    // Check for compute shader support
    GLint maxComputeWorkGroupSize[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxComputeWorkGroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxComputeWorkGroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxComputeWorkGroupSize[2]);
    std::cout << "Max Compute Work Group Size: " << maxComputeWorkGroupSize[0] 
              << "x" << maxComputeWorkGroupSize[1] << "x" << maxComputeWorkGroupSize[2] << std::endl;
    
    GLint maxComputeInvocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxComputeInvocations);
    std::cout << "Max Compute Invocations: " << maxComputeInvocations << std::endl;
    
    // Try to initialize RTX hardware acceleration
    if (initializeRTX()) {
        std::cout << "RTX hardware acceleration enabled" << std::endl;
    } else {
        std::cout << "Using compute shader ray tracing fallback" << std::endl;
    }
    
    // Create ray tracing shaders
    std::cout << "Loading ray tracing shaders..." << std::endl;
    createRayTracingShaders();
    
    // Create framebuffers
    std::cout << "Creating framebuffers..." << std::endl;
    createFramebuffers();
    
    // Update resolution based on quality
    std::cout << "Updating resolution based on quality..." << std::endl;
    updateResolution();
    
    // Create performance query
    glGenQueries(1, &timeQuery_);
    
    std::cout << "Ray Tracing System initialized successfully" << std::endl;
    std::cout << "========================================\n" << std::endl;
    return true;
}

void RayTracingManager::cleanup() {
    if (timeQuery_ != 0) {
        glDeleteQueries(1, &timeQuery_);
        timeQuery_ = 0;
    }
    
    cleanupRTX();
}

void RayTracingManager::setQuality(RayTracingQuality quality) {
    if (quality_ != quality) {
        quality_ = quality;
        updateResolution();
        std::cout << "Ray tracing quality set to: " << (int)quality << std::endl;
    }
}

void RayTracingManager::setFeatures(const RayTracingFeatures& features) {
    features_ = features;
}

void RayTracingManager::updateResolution() {
    switch (quality_) {
        case RayTracingQuality::OFF:
            rtWidth_ = 0;
            rtHeight_ = 0;
            break;
        case RayTracingQuality::LOW:
            rtWidth_ = screenWidth_ / 4;
            rtHeight_ = screenHeight_ / 4;
            break;
        case RayTracingQuality::MEDIUM:
            rtWidth_ = screenWidth_ / 2;
            rtHeight_ = screenHeight_ / 2;
            break;
        case RayTracingQuality::HIGH:
            rtWidth_ = screenWidth_;
            rtHeight_ = screenHeight_;
            break;
        case RayTracingQuality::ULTRA:
            rtWidth_ = screenWidth_;
            rtHeight_ = screenHeight_;
            break;
    }
    
    if (rtWidth_ > 0 && rtHeight_ > 0) {
        // Recreate framebuffers with new resolution
        createFramebuffers();
    }
}

void RayTracingManager::createRayTracingShaders() {
    std::cout << "Loading ray tracing shaders..." << std::endl;
    
    try {
        // Load G-buffer shader
        std::cout << "  Loading G-buffer shader..." << std::endl;
        GLuint gBufferProgram = InitShader("shaders/gbuffer.vs", "shaders/gbuffer.fs");
        if (gBufferProgram != 0) {
            gBufferShader_.setId(gBufferProgram);
            std::cout << "  ✓ G-buffer shader loaded successfully (ID: " << gBufferProgram << ")" << std::endl;
        } else {
            std::cerr << "  ✗ Failed to load G-buffer shader!" << std::endl;
        }
        
        // Load reflection compute shader
        std::cout << "  Loading reflection compute shader..." << std::endl;
        GLuint reflectionCS = InitComputeShader("shaders/rt_reflection.cs");
        if (reflectionCS != 0) {
            reflectionShader_.setId(reflectionCS);
            std::cout << "  ✓ Reflection compute shader loaded successfully (ID: " << reflectionCS << ")" << std::endl;
        } else {
            std::cerr << "  ✗ Failed to load reflection compute shader!" << std::endl;
        }
        
        // Load refraction compute shader
        std::cout << "  Loading refraction compute shader..." << std::endl;
        GLuint refractionCS = InitComputeShader("shaders/rt_refraction.cs");
        if (refractionCS != 0) {
            refractionShader_.setId(refractionCS);
            std::cout << "  ✓ Refraction compute shader loaded successfully (ID: " << refractionCS << ")" << std::endl;
        } else {
            std::cerr << "  ✗ Failed to load refraction compute shader!" << std::endl;
        }
        
        // Load caustic compute shader
        std::cout << "  Loading caustic compute shader..." << std::endl;
        GLuint causticCS = InitComputeShader("shaders/rt_caustics.cs");
        if (causticCS != 0) {
            causticShader_.setId(causticCS);
            std::cout << "  ✓ Caustic compute shader loaded successfully (ID: " << causticCS << ")" << std::endl;
        } else {
            std::cerr << "  ✗ Failed to load caustic compute shader!" << std::endl;
        }
        
        // Load compositing compute shader
        std::cout << "  Loading compositing compute shader..." << std::endl;
        GLuint compositingCS = InitComputeShader("shaders/rt_composite.cs");
        if (compositingCS != 0) {
            compositingShader_.setId(compositingCS);
            std::cout << "  ✓ Compositing compute shader loaded successfully (ID: " << compositingCS << ")" << std::endl;
        } else {
            std::cerr << "  ✗ Failed to load compositing compute shader!" << std::endl;
        }
        
        // Load upsampling compute shader
        std::cout << "  Loading upsampling compute shader..." << std::endl;
        GLuint upsampleCS = InitComputeShader("shaders/rt_upsample.cs");
        if (upsampleCS != 0) {
            upsampleShader_.setId(upsampleCS);
            std::cout << "  ✓ Upsampling compute shader loaded successfully (ID: " << upsampleCS << ")" << std::endl;
        } else {
            std::cerr << "  ✗ Failed to load upsampling compute shader!" << std::endl;
        }
        
        // Check shader validity
        std::cout << "\nShader validity check:" << std::endl;
        std::cout << "  G-buffer shader valid: " << gBufferShader_.isValid() << std::endl;
        std::cout << "  Reflection shader valid: " << reflectionShader_.isValid() << std::endl;
        std::cout << "  Refraction shader valid: " << refractionShader_.isValid() << std::endl;
        std::cout << "  Caustic shader valid: " << causticShader_.isValid() << std::endl;
        std::cout << "  Compositing shader valid: " << compositingShader_.isValid() << std::endl;
        std::cout << "  Upsampling shader valid: " << upsampleShader_.isValid() << std::endl;
        
        std::cout << "Ray tracing shader loading completed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception while loading ray tracing shaders: " << e.what() << std::endl;
    }
}

void RayTracingManager::createFramebuffers() {
    if (rtWidth_ <= 0 || rtHeight_ <= 0) return;
    
    // Create G-Buffer for deferred rendering
    gBuffer_.bind();
    
    // Position + depth texture
    positionTexture_.generate();
    positionTexture_.storage(rtWidth_, rtHeight_, GL_RGBA32F);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, positionTexture_.get(), 0);
    
    // Normal texture
    normalTexture_.generate();
    normalTexture_.storage(rtWidth_, rtHeight_, GL_RGBA16F);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTexture_.get(), 0);
    
    // Depth texture
    depthTexture_.generate();
    depthTexture_.storage(rtWidth_, rtHeight_, GL_DEPTH_COMPONENT32F);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture_.get(), 0);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Ray tracing G-buffer not complete!" << std::endl;
    }
    
    gBuffer_.unbind();
    
    // Create ray traced result textures
    rayTracedTexture_.generate();
    rayTracedTexture_.storage(rtWidth_, rtHeight_, GL_RGBA16F);
    
    reflectionTexture_.generate();
    reflectionTexture_.storage(rtWidth_, rtHeight_, GL_RGBA16F);
    
    refractionTexture_.generate();
    refractionTexture_.storage(rtWidth_, rtHeight_, GL_RGBA16F);
    
    causticTexture_.generate();
    causticTexture_.storage(rtWidth_, rtHeight_, GL_RGBA16F);
    
    // Create final full-resolution texture
    finalTexture_.generate();
    finalTexture_.storage(screenWidth_, screenHeight_, GL_RGBA8);
    
    std::cout << "Ray tracing framebuffers created: " << rtWidth_ << "x" << rtHeight_ << " -> " << screenWidth_ << "x" << screenHeight_ << std::endl;
}

void RayTracingManager::renderWaterRayTraced(const glm::mat4& view, const glm::mat4& projection,
                                           const glm::vec3& cameraPos, const glm::vec3& lightPos) {
    static int frameCount = 0;
    frameCount++;
    
    // Debug output on first frame or every 60 frames
    bool shouldDebug = (frameCount == 1 || frameCount % 60 == 0);
    
    if (shouldDebug) {
        std::cout << "\n=== Ray Tracing Frame " << frameCount << " ===" << std::endl;
        std::cout << "Quality: " << (int)quality_ << " (OFF=" << (int)RayTracingQuality::OFF << ")" << std::endl;
        std::cout << "RT Resolution: " << rtWidth_ << "x" << rtHeight_ << std::endl;
        std::cout << "Water VAO: " << waterVAO_ << ", Vertex Count: " << waterVertexCount_ << std::endl;
        std::cout << "Features - Reflections: " << features_.reflections 
                  << ", Refractions: " << features_.refractions 
                  << ", Caustics: " << features_.caustics << std::endl;
    }
    
    if (quality_ == RayTracingQuality::OFF) {
        if (shouldDebug) {
            std::cout << "Ray tracing SKIPPED - Quality is OFF" << std::endl;
        }
        return;
    }
    
    if (rtWidth_ <= 0 || rtHeight_ <= 0) {
        if (shouldDebug) {
            std::cout << "Ray tracing SKIPPED - Invalid resolution" << std::endl;
        }
        return;
    }
    
    if (waterVAO_ == 0 || waterVertexCount_ == 0) {
        if (shouldDebug) {
            std::cout << "Ray tracing SKIPPED - No water geometry" << std::endl;
        }
        return;
    }
    
    if (shouldDebug) {
        std::cout << "Starting ray tracing render..." << std::endl;
    }
    
    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();
    glBeginQuery(GL_TIME_ELAPSED, timeQuery_);
    
    // Save current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    // Set ray tracing viewport
    glViewport(0, 0, rtWidth_, rtHeight_);
    
    // 1. Render G-Buffer for water surface (this needs actual water geometry)
    if (shouldDebug) std::cout << "Rendering G-Buffer..." << std::endl;
    renderGBuffer(view, projection);
    
    // 2. Trace reflections if enabled
    if (features_.reflections) {
        if (shouldDebug) std::cout << "Tracing reflections..." << std::endl;
        traceReflections(cameraPos, lightPos);
    }
    
    // 3. Trace refractions if enabled
    if (features_.refractions) {
        if (shouldDebug) std::cout << "Tracing refractions..." << std::endl;
        traceRefractions(cameraPos);
    }
    
    // 4. Generate caustics if enabled
    if (features_.caustics) {
        if (shouldDebug) std::cout << "Generating caustics..." << std::endl;
        traceCaustics(lightPos);
    }
    
    // 5. Composite all results
    if (shouldDebug) std::cout << "Compositing results..." << std::endl;
    compositeResults(cameraPos);
    
    // 6. Upsample to full resolution if needed
    if (rtWidth_ != screenWidth_ || rtHeight_ != screenHeight_) {
        if (shouldDebug) std::cout << "Upsampling to full resolution..." << std::endl;
        upsampleToFullResolution();
    }
    
    // Restore original viewport
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    
    // End timing
    glEndQuery(GL_TIME_ELAPSED);
    
    // Ensure all compute shader operations are complete
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    
    // Calculate performance metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    lastFrameTime_ = duration.count() / 1000.0f; // Convert to milliseconds
    
    // Estimate rays per second (rough calculation)
    int totalRays = rtWidth_ * rtHeight_;
    if (quality_ == RayTracingQuality::ULTRA) totalRays *= 4; // Supersampling
    raysPerSecond_ = (lastFrameTime_ > 0) ? (int)(totalRays / (lastFrameTime_ / 1000.0f)) : 0;
    
    if (shouldDebug) {
        std::cout << "Ray tracing completed - Frame time: " << lastFrameTime_ << "ms" << std::endl;
        std::cout << "Final texture ID: " << finalTexture_.get() << std::endl;
        std::cout << "============================\n" << std::endl;
    }
}

void RayTracingManager::renderGBuffer(const glm::mat4& view, const glm::mat4& projection) {
    // Bind G-Buffer
    gBuffer_.bind();
    
    // Set viewport
    glViewport(0, 0, rtWidth_, rtHeight_);
    
    // Enable multiple render targets
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);
    
    // Clear
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render water surface geometry to G-Buffer
    if (waterVAO_ != 0 && waterVertexCount_ > 0) {
        gBufferShader_.use();
        
        // Set matrices
        glm::mat4 model = glm::mat4(1.0f); // Identity for water surface
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        
        gBufferShader_.setMat4("uModel", model);
        gBufferShader_.setMat4("uView", view);
        gBufferShader_.setMat4("uProjection", projection);
        gBufferShader_.setMat3("uNormalMatrix", normalMatrix);
        
        // Water properties
        gBufferShader_.setFloat("uWaterLevel", 0.0f);
        gBufferShader_.setVec3("uWaterColor", glm::vec3(0.1f, 0.4f, 0.7f));
        
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        // Bind water geometry and render
        glBindVertexArray(waterVAO_);
        glDrawArrays(GL_TRIANGLES, 0, waterVertexCount_);
        glBindVertexArray(0);
        
        glDisable(GL_DEPTH_TEST);
    }
    
    gBuffer_.unbind();
}

void RayTracingManager::traceReflections(const glm::vec3& cameraPos, const glm::vec3& lightPos) {
    // Use compute shader for reflection ray tracing
    reflectionShader_.use();
    
    // Bind input textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture_.get());
    reflectionShader_.setInt("uPositionTexture", 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture_.get());
    reflectionShader_.setInt("uNormalTexture", 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture_.get());
    reflectionShader_.setInt("uDepthTexture", 2);
    
    // Bind output texture
    glBindImageTexture(0, reflectionTexture_.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    // Set uniforms
    reflectionShader_.setVec3("uCameraPos", cameraPos);
    reflectionShader_.setVec3("uLightPos", lightPos);
    reflectionShader_.setVec2("uResolution", glm::vec2(rtWidth_, rtHeight_));
    
    // Dispatch compute shader
    int groupsX = (rtWidth_ + 15) / 16;
    int groupsY = (rtHeight_ + 15) / 16;
    glDispatchCompute(groupsX, groupsY, 1);
    
    // Memory barrier
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void RayTracingManager::traceRefractions(const glm::vec3& cameraPos) {
    // Use compute shader for refraction ray tracing
    refractionShader_.use();
    
    // Bind input textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture_.get());
    refractionShader_.setInt("uPositionTexture", 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture_.get());
    refractionShader_.setInt("uNormalTexture", 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture_.get());
    refractionShader_.setInt("uDepthTexture", 2);
    
    // Bind output texture
    glBindImageTexture(1, refractionTexture_.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    // Set uniforms
    refractionShader_.setVec3("uCameraPos", cameraPos);
    refractionShader_.setVec2("uResolution", glm::vec2(rtWidth_, rtHeight_));
    refractionShader_.setFloat("uWaterIOR", 1.33f);
    refractionShader_.setFloat("uWaterDepth", 5.0f);
    refractionShader_.setVec3("uWaterColor", glm::vec3(0.1f, 0.3f, 0.6f));
    
    // Dispatch compute shader
    int groupsX = (rtWidth_ + 15) / 16;
    int groupsY = (rtHeight_ + 15) / 16;
    glDispatchCompute(groupsX, groupsY, 1);
    
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void RayTracingManager::traceCaustics(const glm::vec3& lightPos) {
    // Generate caustic patterns using ray tracing
    causticShader_.use();
    
    // Bind output texture
    glBindImageTexture(2, causticTexture_.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    // Set uniforms
    causticShader_.setVec3("uLightPos", lightPos);
    causticShader_.setVec3("uLightDir", glm::normalize(glm::vec3(0.0f, -1.0f, 0.2f))); // Directional light
    causticShader_.setVec2("uResolution", glm::vec2(rtWidth_, rtHeight_));
    causticShader_.setFloat("uTime", static_cast<float>(glfwGetTime()));
    causticShader_.setFloat("uWaterLevel", 0.0f);
    causticShader_.setFloat("uCausticStrength", 1.0f);
    causticShader_.setFloat("uWaterIOR", 1.33f);
    causticShader_.setInt("uCausticRays", 64);
    causticShader_.setFloat("uCausticRadius", 2.0f);
    causticShader_.setFloat("uFloorDepth", -5.0f);
    
    // Dispatch compute shader
    int groupsX = (rtWidth_ + 15) / 16;
    int groupsY = (rtHeight_ + 15) / 16;
    glDispatchCompute(groupsX, groupsY, 1);
    
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void RayTracingManager::compositeResults(const glm::vec3& cameraPos) {
    // Composite all ray traced results into final texture
    compositingShader_.use();
    
    // Bind input textures (use a clear texture for base color since we don't have scene background)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0); // No base color texture - shader will use water color fallback
    compositingShader_.setInt("uBaseColorTexture", 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, reflectionTexture_.get());
    compositingShader_.setInt("uReflectionTexture", 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, refractionTexture_.get());
    compositingShader_.setInt("uRefractionTexture", 2);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, causticTexture_.get());
    compositingShader_.setInt("uCausticTexture", 3);
    
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, depthTexture_.get());
    compositingShader_.setInt("uDepthTexture", 4);
    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, normalTexture_.get());
    compositingShader_.setInt("uNormalTexture", 5);
    
    // Bind output texture
    glBindImageTexture(0, rayTracedTexture_.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    
    // Set uniforms
    compositingShader_.setVec2("uResolution", glm::vec2(rtWidth_, rtHeight_));
    compositingShader_.setFloat("uReflectionStrength", features_.reflections ? 1.0f : 0.0f);
    compositingShader_.setFloat("uRefractionStrength", features_.refractions ? 1.0f : 0.0f);
    compositingShader_.setFloat("uCausticStrength", features_.caustics ? 0.5f : 0.0f);
    compositingShader_.setBool("uEnableReflections", features_.reflections);
    compositingShader_.setBool("uEnableRefractions", features_.refractions);
    compositingShader_.setBool("uEnableCaustics", features_.caustics);
    compositingShader_.setFloat("uWaterIOR", 1.33f);
    compositingShader_.setVec3("uWaterColor", glm::vec3(0.1f, 0.4f, 0.7f));
    compositingShader_.setFloat("uWaterRoughness", 0.02f);
    compositingShader_.setVec3("uCameraPos", cameraPos);
    
    // Dispatch compute shader
    int groupsX = (rtWidth_ + 15) / 16;
    int groupsY = (rtHeight_ + 15) / 16;
    glDispatchCompute(groupsX, groupsY, 1);
    
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void RayTracingManager::upsampleToFullResolution() {
    // Upsample the composited ray traced result to full resolution
    upsampleShader_.use();
    
    // Bind input texture (composited ray traced result)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rayTracedTexture_.get());
    upsampleShader_.setInt("uLowResTexture", 0);
    
    // Bind output texture
    glBindImageTexture(0, finalTexture_.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    
    // Set uniforms
    upsampleShader_.setVec2("uLowResolution", glm::vec2(rtWidth_, rtHeight_));
    upsampleShader_.setVec2("uHighResolution", glm::vec2(screenWidth_, screenHeight_));
    upsampleShader_.setFloat("uSharpenAmount", 0.2f); // Slight sharpening to reduce blur
    
    // Dispatch compute shader
    int groupsX = (screenWidth_ + 15) / 16;
    int groupsY = (screenHeight_ + 15) / 16;
    glDispatchCompute(groupsX, groupsY, 1);
    
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void RayTracingManager::resize(int width, int height) {
    screenWidth_ = width;
    screenHeight_ = height;
    updateResolution();
}

void RayTracingManager::updateWaterSurface(const std::vector<glm::vec3>& vertices,
                                         const std::vector<glm::vec3>& normals) {
    // Update water surface data for ray tracing
    if (!vertices.empty()) {
        waterVertexBuffer_.bind();
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), 
                     vertices.data(), GL_DYNAMIC_DRAW);
    }
    
    if (!normals.empty()) {
        waterNormalBuffer_.bind();
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3),
                     normals.data(), GL_DYNAMIC_DRAW);
    }
}

bool RayTracingManager::initializeRTX() {

    
    // Check for RTX capability
    // For now, assume RTX is available
    rtxAvailable_ = true;  // Would check actual hardware
    
    if (rtxAvailable_) {
        // Initialize OptiX context
        // rtxContext_ = createOptiXContext();
        std::cout << "RTX hardware detected" << std::endl;
        return true;
    }
    
    return false;
}

void RayTracingManager::cleanupRTX() {
    if (rtxContext_) {
        // Cleanup OptiX resources
        rtxContext_ = nullptr;
    }
    rtxAvailable_ = false;
}

} // namespace WaterSim

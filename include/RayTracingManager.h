#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "GLResources.h"
#include "Config.h"

namespace WaterSim {

// Ray tracing quality levels for real-time performance
enum class RayTracingQuality {
    OFF = 0,
    LOW = 1,        // 1/4 resolution, 1 ray per pixel
    MEDIUM = 2,     // 1/2 resolution, 1 ray per pixel  
    HIGH = 3,       // Full resolution, 1 ray per pixel
    ULTRA = 4       // Full resolution, 4 rays per pixel (supersampling)
};

// Ray tracing feature flags
struct RayTracingFeatures {
    bool reflections = true;
    bool refractions = true;
    bool caustics = true;
    bool volumetricLighting = false;
    bool softShadows = true;
    bool globalIllumination = false;
};

class RayTracingManager {
public:
    RayTracingManager(const Config& config);
    ~RayTracingManager();
    
    // Initialize ray tracing system
    bool initialize(int screenWidth, int screenHeight);
    void cleanup();
    
    // Main rendering pipeline
    void renderWaterRayTraced(const glm::mat4& view, const glm::mat4& projection,
                             const glm::vec3& cameraPos, const glm::vec3& lightPos);
    
    // Quality and feature control
    void setQuality(RayTracingQuality quality);
    void setFeatures(const RayTracingFeatures& features);
    RayTracingQuality getQuality() const { return quality_; }
    
    // Performance monitoring
    float getLastFrameTime() const { return lastFrameTime_; }
    int getRaysPerSecond() const { return raysPerSecond_; }
    
    // Screen space settings
    void resize(int width, int height);
    
    // Water surface data input
    void updateWaterSurface(const std::vector<glm::vec3>& vertices,
                           const std::vector<glm::vec3>& normals);
    
    // Result texture for compositing
    GLuint getRayTracedTexture() const { return finalTexture_.get(); }
    GLuint getReflectionTexture() const { return reflectionTexture_.get(); }
    GLuint getRefractionTexture() const { return refractionTexture_.get(); }
    GLuint getCausticTexture() const { return causticTexture_.get(); }
    
    // Set water geometry for G-buffer rendering
    void setWaterGeometry(GLuint waterVAO, int vertexCount) {
        waterVAO_ = waterVAO;
        waterVertexCount_ = vertexCount;
    }
    
private:
    const Config& config_;
    RayTracingQuality quality_;
    RayTracingFeatures features_;
    
    // Screen dimensions
    int screenWidth_, screenHeight_;
    int rtWidth_, rtHeight_;  // Ray tracing resolution
    
    // GPU resources
    GLTexture2D rayTracedTexture_;
    GLTexture2D finalTexture_;  // Full resolution output
    GLTexture2D depthTexture_;
    GLTexture2D normalTexture_;
    GLTexture2D positionTexture_;
    GLTexture2D reflectionTexture_;
    GLTexture2D refractionTexture_;
    GLTexture2D causticTexture_;
    GLFramebuffer gBuffer_;
    
    // Water geometry for G-buffer rendering
    GLuint waterVAO_ = 0;
    int waterVertexCount_ = 0;
    
    // G-buffer shader
    GLShaderProgram gBufferShader_;
    
    // Ray tracing compute shaders
    GLShaderProgram rayGenShader_;
    GLShaderProgram reflectionShader_;
    GLShaderProgram refractionShader_;
    GLShaderProgram causticShader_;
    GLShaderProgram compositingShader_;
    GLShaderProgram upsampleShader_;
    
    // Water surface data
    GLBuffer waterVertexBuffer_{GL_ARRAY_BUFFER};
    GLBuffer waterNormalBuffer_{GL_ARRAY_BUFFER};
    GLTexture2D heightMapTexture_;
    GLTexture2D normalMapTexture_;
    
    // Performance tracking
    float lastFrameTime_;
    int raysPerSecond_;
    GLuint timeQuery_;
    
    // Private methods
    void createRayTracingShaders();
    void createFramebuffers();
    void updateResolution();
    void renderGBuffer(const glm::mat4& view, const glm::mat4& projection);
    void traceReflections(const glm::vec3& cameraPos, const glm::vec3& lightPos);
    void traceRefractions(const glm::vec3& cameraPos);
    void traceCaustics(const glm::vec3& lightPos);
    void compositeResults(const glm::vec3& cameraPos);
    void upsampleToFullResolution();
    
    // RTX hardware acceleration (if available)
    bool initializeRTX();
    void cleanupRTX();
    bool rtxAvailable_;
    void* rtxContext_;  // OptiX context
};

} // namespace WaterSim
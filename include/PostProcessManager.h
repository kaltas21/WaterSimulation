#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Framebuffer.h"

class PostProcessManager {
public:
    PostProcessManager(int width, int height);
    ~PostProcessManager();

    // Begin/end post-processing pass
    void beginPostProcess();
    void endPostProcess();
    
    // Apply effects
    void applyPostProcessing(GLuint inputTexture, GLuint depthTexture = 0);
    
    // Effect toggles
    void setBloomEnabled(bool enabled) { bloomEnabled = enabled; }
    void setDOFEnabled(bool enabled) { dofEnabled = enabled; }
    void setVolumetricLightingEnabled(bool enabled) { volumetricEnabled = enabled; }
    
    // Effect parameters
    void setBloomParams(float threshold, float intensity) { 
        bloomThreshold = threshold; 
        bloomIntensity = intensity; 
    }
    void setDOFParams(float focusDistance, float focusRange) {
        this->focusDistance = focusDistance;
        this->focusRange = focusRange;
    }
    
    // Resize
    void resize(int width, int height);

private:
    Framebuffer* postProcessFBO;
    GLuint postProcessShader;
    GLuint quadVAO, quadVBO;
    
    int width, height;
    
    // Effect parameters
    bool bloomEnabled = true;
    bool dofEnabled = false;
    bool volumetricEnabled = false;  // Disable volumetric lighting to reduce brightness
    
    float bloomThreshold = 2.0f;    // Higher threshold = less bloom
    float bloomIntensity = 0.2f;    // Much lower intensity
    float focusDistance = 10.0f;
    float focusRange = 5.0f;
    
    void setupQuad();
    void loadShaders();
};
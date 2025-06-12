#include "../include/ReflectionRenderer.h"
#include <iostream>

ReflectionRenderer::ReflectionRenderer(int width, int height)
    : width(width), height(height) {
    
    // Create framebuffers for reflection and refraction
    reflectionFBO = new Framebuffer(width, height, FRAMEBUFFER_COLOR_DEPTH);
    refractionFBO = new Framebuffer(width, height, FRAMEBUFFER_COLOR_DEPTH);
    
    if (!reflectionFBO->isComplete() || !refractionFBO->isComplete()) {
        std::cerr << "Failed to create reflection/refraction framebuffers!" << std::endl;
    }
}

ReflectionRenderer::~ReflectionRenderer() {
    delete reflectionFBO;
    delete refractionFBO;
}

void ReflectionRenderer::beginReflectionRender(const Camera& camera, float waterLevel) {
    // Bind reflection framebuffer
    reflectionFBO->bind();
    
    // Clear with sky color
    glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // Light blue sky
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Create reflection matrix
    reflectionMatrix = createReflectionMatrix(waterLevel);
    
    // Setup clipping plane to clip geometry below water
    glm::vec4 clipPlane(0.0f, 1.0f, 0.0f, -waterLevel + 0.1f);
    setupClippingPlane(clipPlane);
    
    // Enable clipping
    glEnable(GL_CLIP_DISTANCE0);
}

void ReflectionRenderer::endReflectionRender() {
    // Disable clipping
    glDisable(GL_CLIP_DISTANCE0);
    
    // Unbind framebuffer
    reflectionFBO->unbind();
    
    // Reset viewport to screen size
    glViewport(0, 0, width, height);
}

void ReflectionRenderer::beginRefractionRender(const Camera& camera, float waterLevel) {
    // Bind refraction framebuffer
    refractionFBO->bind();
    
    // Clear with dark underwater color
    glClearColor(0.0f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Setup clipping plane to clip geometry above water
    glm::vec4 clipPlane(0.0f, -1.0f, 0.0f, waterLevel + 0.1f);
    setupClippingPlane(clipPlane);
    
    // Enable clipping
    glEnable(GL_CLIP_DISTANCE0);
}

void ReflectionRenderer::endRefractionRender() {
    // Disable clipping
    glDisable(GL_CLIP_DISTANCE0);
    
    // Unbind framebuffer
    refractionFBO->unbind();
    
    // Reset viewport to screen size
    glViewport(0, 0, width, height);
}

GLuint ReflectionRenderer::getReflectionTexture() const {
    return reflectionFBO->getColorTexture();
}

GLuint ReflectionRenderer::getRefractionTexture() const {
    return refractionFBO->getColorTexture();
}

void ReflectionRenderer::resize(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
    
    reflectionFBO->resize(width, height);
    refractionFBO->resize(width, height);
}

glm::mat4 ReflectionRenderer::createReflectionMatrix(float waterLevel) {
    // Create reflection matrix across the water plane (y = waterLevel)
    glm::mat4 reflection(1.0f);
    reflection[1][1] = -1.0f;  // Flip Y coordinate
    reflection[3][1] = 2.0f * waterLevel;  // Translate to water level
    
    return reflection;
}

void ReflectionRenderer::setupClippingPlane(const glm::vec4& plane) {
}
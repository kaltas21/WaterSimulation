#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Framebuffer.h"
#include "Camera.h"

class ReflectionRenderer {
public:
    ReflectionRenderer(int width, int height);
    ~ReflectionRenderer();

    // Begin/end reflection rendering
    void beginReflectionRender(const Camera& camera, float waterLevel = 0.0f);
    void endReflectionRender();
    
    // Begin/end refraction rendering
    void beginRefractionRender(const Camera& camera, float waterLevel = 0.0f);
    void endRefractionRender();
    
    // Get texture IDs for binding to water shader
    GLuint getReflectionTexture() const;
    GLuint getRefractionTexture() const;
    
    // Resize framebuffers
    void resize(int width, int height);

private:
    Framebuffer* reflectionFBO;
    Framebuffer* refractionFBO;
    
    glm::mat4 reflectionMatrix;
    glm::mat4 refractionMatrix;
    
    int width, height;
    
    // Create reflection matrix for water plane
    glm::mat4 createReflectionMatrix(float waterLevel);
    
    // Setup clipping planes for refraction/reflection
    void setupClippingPlane(const glm::vec4& plane);
};
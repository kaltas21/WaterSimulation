#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>

enum FramebufferType {
    FRAMEBUFFER_COLOR_DEPTH,
    FRAMEBUFFER_COLOR_ONLY,
    FRAMEBUFFER_DEPTH_ONLY,
    FRAMEBUFFER_MULTISAMPLED
};

class Framebuffer {
public:
    Framebuffer(int width, int height, FramebufferType type = FRAMEBUFFER_COLOR_DEPTH, int samples = 1);
    ~Framebuffer();

    // Bind/unbind framebuffer
    void bind() const;
    void unbind() const;

    // Resize framebuffer
    void resize(int width, int height);

    // Get texture IDs
    GLuint getColorTexture() const { return colorTexture; }
    GLuint getDepthTexture() const { return depthTexture; }

    // Get dimensions
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Clear framebuffer
    void clear(const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    // Check if framebuffer is complete
    bool isComplete() const;

    // Blit multisampled framebuffer to regular framebuffer
    void blitTo(Framebuffer* target) const;

private:
    GLuint fbo;
    GLuint colorTexture;
    GLuint depthTexture;
    GLuint depthRenderbuffer;
    
    int width;
    int height;
    FramebufferType type;
    int samples;

    void create();
    void destroy();
};
#include "../include/Framebuffer.h"

Framebuffer::Framebuffer(int width, int height, FramebufferType type, int samples)
    : width(width), height(height), type(type), samples(samples),
      fbo(0), colorTexture(0), depthTexture(0), depthRenderbuffer(0) {
    create();
}

Framebuffer::~Framebuffer() {
    destroy();
}

void Framebuffer::create() {
    // Generate framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create color attachment
    if (type != FRAMEBUFFER_DEPTH_ONLY) {
        if (samples > 1 && type == FRAMEBUFFER_MULTISAMPLED) {
            // Multisampled color texture
            glGenTextures(1, &colorTexture);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorTexture);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA16F, width, height, GL_TRUE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, colorTexture, 0);
        } else {
            // Regular color texture with float precision for HDR
            glGenTextures(1, &colorTexture);
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        }
    }

    // Create depth attachment
    if (type != FRAMEBUFFER_COLOR_ONLY) {
        if (type == FRAMEBUFFER_DEPTH_ONLY || (type == FRAMEBUFFER_COLOR_DEPTH && samples == 1)) {
            // Depth texture (can be sampled in shaders)
            glGenTextures(1, &depthTexture);
            glBindTexture(GL_TEXTURE_2D, depthTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
        } else {
            // Depth renderbuffer (cannot be sampled, but faster)
            glGenRenderbuffers(1, &depthRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
            if (samples > 1 && type == FRAMEBUFFER_MULTISAMPLED) {
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
            } else {
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
            }
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
        }
    }

    // Set draw buffers
    if (type == FRAMEBUFFER_DEPTH_ONLY) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    // Check framebuffer completeness
    if (!isComplete()) {
        std::cerr << "Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::destroy() {
    if (colorTexture) {
        glDeleteTextures(1, &colorTexture);
        colorTexture = 0;
    }
    if (depthTexture) {
        glDeleteTextures(1, &depthTexture);
        depthTexture = 0;
    }
    if (depthRenderbuffer) {
        glDeleteRenderbuffers(1, &depthRenderbuffer);
        depthRenderbuffer = 0;
    }
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
}

void Framebuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(int newWidth, int newHeight) {
    if (width == newWidth && height == newHeight) return;
    
    width = newWidth;
    height = newHeight;
    
    destroy();
    create();
}

void Framebuffer::clear(const glm::vec4& clearColor) {
    bind();
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    unbind();
}

bool Framebuffer::isComplete() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        switch (status) {
            case GL_FRAMEBUFFER_UNDEFINED:
                std::cerr << "Framebuffer undefined" << std::endl;
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                std::cerr << "Framebuffer incomplete attachment" << std::endl;
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                std::cerr << "Framebuffer missing attachment" << std::endl;
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                std::cerr << "Framebuffer unsupported" << std::endl;
                break;
            default:
                std::cerr << "Framebuffer unknown error: " << status << std::endl;
                break;
        }
        return false;
    }
    return true;
}

void Framebuffer::blitTo(Framebuffer* target) const {
    GLint srcFbo = fbo;
    GLint dstFbo = target ? target->fbo : 0;
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFbo);
    
    int dstWidth = target ? target->width : width;
    int dstHeight = target ? target->height : height;
    
    glBlitFramebuffer(
        0, 0, width, height,
        0, 0, dstWidth, dstHeight,
        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
        GL_LINEAR
    );
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
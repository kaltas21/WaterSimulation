#include "../include/PostProcessManager.h"
#include "../include/InitShader.h"
#include <GLFW/glfw3.h>
#include <iostream>

PostProcessManager::PostProcessManager(int width, int height)
    : width(width), height(height) {
    
    postProcessFBO = new Framebuffer(width, height, FRAMEBUFFER_COLOR_ONLY);
    
    setupQuad();
    loadShaders();
}

PostProcessManager::~PostProcessManager() {
    delete postProcessFBO;
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    if (postProcessShader) {
        glDeleteProgram(postProcessShader);
    }
}

void PostProcessManager::setupQuad() {
    // Full-screen quad vertices
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

void PostProcessManager::loadShaders() {
    try {
        postProcessShader = InitShader("shaders/postprocess.vs", "shaders/postprocess.fs");
        
        if (postProcessShader == 0) {
            std::cerr << "Failed to load post-process shaders!" << std::endl;
            return;
        }
        
        // Set uniform locations
        glUseProgram(postProcessShader);
        glUniform1i(glGetUniformLocation(postProcessShader, "screenTexture"), 0);
        glUniform1i(glGetUniformLocation(postProcessShader, "depthTexture"), 1);
        glUniform1i(glGetUniformLocation(postProcessShader, "refractionTexture"), 2);
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading post-process shaders: " << e.what() << std::endl;
        postProcessShader = 0;
    }
}

void PostProcessManager::beginPostProcess() {
    postProcessFBO->bind();
    glClear(GL_COLOR_BUFFER_BIT);
}

void PostProcessManager::endPostProcess() {
    postProcessFBO->unbind();
}

void PostProcessManager::applyPostProcessing(GLuint inputTexture, GLuint depthTexture) {
    if (postProcessShader == 0) return;
    
    // Bind shader
    glUseProgram(postProcessShader);
    
    // Disable depth testing for post-processing
    glDisable(GL_DEPTH_TEST);
    
    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    
    if (depthTexture) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
    }
    
    // Set uniforms
    glUniform2f(glGetUniformLocation(postProcessShader, "resolution"), 
                static_cast<float>(width), static_cast<float>(height));
    
    glUniform1f(glGetUniformLocation(postProcessShader, "time"), 
                static_cast<float>(glfwGetTime()));
    
    // Effect toggles
    glUniform1i(glGetUniformLocation(postProcessShader, "enableBloom"), bloomEnabled);
    glUniform1i(glGetUniformLocation(postProcessShader, "enableDOF"), dofEnabled);
    glUniform1i(glGetUniformLocation(postProcessShader, "enableVolumetricLighting"), volumetricEnabled);
    
    // Effect parameters
    glUniform1f(glGetUniformLocation(postProcessShader, "bloomThreshold"), bloomThreshold);
    glUniform1f(glGetUniformLocation(postProcessShader, "bloomIntensity"), bloomIntensity);
    glUniform1f(glGetUniformLocation(postProcessShader, "focusDistance"), focusDistance);
    glUniform1f(glGetUniformLocation(postProcessShader, "focusRange"), focusRange);
    
    // Render full-screen quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);
}
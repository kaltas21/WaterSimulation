#include "../include/HeightMapTexture.h"
#include <iostream>

HeightMapTexture::HeightMapTexture(int width, int height)
    : width(width), height(height), textureID(0) {
    create();
}

HeightMapTexture::~HeightMapTexture() {
    destroy();
}

void HeightMapTexture::create() {
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Create float texture for precise height data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void HeightMapTexture::destroy() {
    if (textureID) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
}

void HeightMapTexture::updateHeightMap(const std::vector<float>& heights) {
    if (heights.size() != static_cast<size_t>(width * height)) {
        std::cerr << "HeightMapTexture: Height data size mismatch!" << std::endl;
        return;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_FLOAT, heights.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void HeightMapTexture::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureID);
}
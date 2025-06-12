#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class HeightMapTexture {
public:
    HeightMapTexture(int width, int height);
    ~HeightMapTexture();

    // Update the height map with wave data
    void updateHeightMap(const std::vector<float>& heights);
    
    // Bind texture for use in shaders
    void bind(unsigned int unit = 0) const;
    
    // Get texture ID
    GLuint getTextureID() const { return textureID; }
    
    // Get dimensions
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    GLuint textureID;
    int width;
    int height;
    
    void create();
    void destroy();
};
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class GlassContainer {
public:
    GlassContainer(float width = 10.0f, float height = 10.0f, float depth = 10.0f);
    ~GlassContainer();

    void initialize();
    void render(unsigned int shaderProgram);

    // Getters and setters
    void setDimensions(float width, float height, float depth);
    void setPosition(const glm::vec3& pos) { position = pos; }
    glm::vec3 getPosition() const { return position; }
    void setColor(const glm::vec3& color) { glassColor = color; }
    glm::vec3 getColor() const { return glassColor; }
    void setTransparency(float alpha) { transparency = alpha; }
    float getTransparency() const { return transparency; }
    void setRefractionIndex(float ri) { refractionIndex = ri; }
    float getRefractionIndex() const { return refractionIndex; }
    
    float getWidth() const { return width; }
    float getHeight() const { return height; }
    float getDepth() const { return depth; }

private:
    unsigned int VAO, VBO, EBO;
    
    // Dimensions
    float width;
    float height;
    float depth;
    
    // Position
    glm::vec3 position;
    
    // Visual properties
    glm::vec3 glassColor;
    float transparency;
    float refractionIndex;
    
    // Geometry data
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Helper methods
    void generateMesh();
}; 
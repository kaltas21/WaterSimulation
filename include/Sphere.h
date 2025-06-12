#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Sphere {
public:
    Sphere(float radius = 1.0f, int sectorCount = 36, int stackCount = 18);
    ~Sphere();

    void initialize();
    void update(float deltaTime);
    void render(unsigned int shaderProgram);

    // Getters and setters
    void setPosition(const glm::vec3& pos) { position = pos; }
    glm::vec3 getPosition() const { return position; }
    
    void setVelocity(const glm::vec3& vel) { velocity = vel; }
    glm::vec3 getVelocity() const { return velocity; }
    
    void setColor(const glm::vec3& color) { sphereColor = color; }
    glm::vec3 getColor() const { return sphereColor; }
    
    float getRadius() const { return radius; }
    
    // Physics
    void applyForce(const glm::vec3& force);
    void applyGravity(float gravity);
    void setUseGravity(bool use) { useGravity = use; }
    bool getUseGravity() const { return useGravity; }
    void setMass(float m) { mass = m; }
    float getMass() const { return mass; }
    void setDrag(float d) { drag = d; }
    
    // Interaction
    void setDragged(bool isDragged) { dragged = isDragged; }
    bool isDragged() const { return dragged; }

private:
    // Geometry
    unsigned int VAO, VBO, EBO;
    float radius;
    int sectorCount;
    int stackCount;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Physics properties
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float mass;
    float drag;
    bool useGravity;
    
    // Visual properties
    glm::vec3 sphereColor;
    
    // Interaction
    bool dragged;
    
    // Helper methods
    void generateVertices();
    void generateIndices();
    
    // Calculate vertices with normals and texture coordinates
    void buildVertices();
    std::vector<float> getUnitCircleVertices();
}; 
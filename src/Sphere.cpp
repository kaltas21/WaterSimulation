#include "../include/Sphere.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <cmath>

Sphere::Sphere(float radius, int sectorCount, int stackCount)
    : radius(radius), sectorCount(sectorCount), stackCount(stackCount),
      position(0.0f), velocity(0.0f), acceleration(0.0f),
      mass(1.0f), drag(0.1f), useGravity(false),
      sphereColor(0.3f, 0.7f, 0.9f), dragged(false) {
}

Sphere::~Sphere() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Sphere::initialize() {
    buildVertices();
    
    // Create and bind VAO and VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Sphere::buildVertices() {
    vertices.clear();
    indices.clear();
    
    std::vector<float> unitCircleVertices = getUnitCircleVertices();
    
    // Generate vertices for sphere using spherical coordinates
    for (int i = 0; i <= stackCount; ++i) {
        // Latitude (stack)
        float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stackCount; // Starting from 90 degrees to -90 degrees
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);
        
        // Save vertex data for current stack
        for (int j = 0; j <= sectorCount; ++j) {
            // Longitude (sector)
            float sectorAngle = j * 2 * glm::pi<float>() / sectorCount; // Starting from 0 to 360 degrees
            
            // Vertex position
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            
            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            // Normal (normalized position)
            float nx = x / radius;
            float ny = y / radius;
            float nz = z / radius;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
            
            // Texture coordinates
            float s = (float)j / sectorCount;
            float t = (float)i / stackCount;
            vertices.push_back(s);
            vertices.push_back(t);
        }
    }
    
    // Generate indices for sphere
    for (int i = 0; i < stackCount; ++i) {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;
        
        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            
            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

std::vector<float> Sphere::getUnitCircleVertices() {
    const float PI = glm::pi<float>();
    float sectorStep = 2 * PI / sectorCount;
    
    std::vector<float> unitCircleVertices;
    for (int i = 0; i <= sectorCount; ++i) {
        float sectorAngle = i * sectorStep;
        unitCircleVertices.push_back(cos(sectorAngle)); // x
        unitCircleVertices.push_back(sin(sectorAngle)); // y
    }
    
    return unitCircleVertices;
}

void Sphere::applyForce(const glm::vec3& force) {
    // F = ma, so a = F/m
    acceleration += force / mass;
}

void Sphere::applyGravity(float gravity) {
    if (useGravity) {
        applyForce(glm::vec3(0.0f, -gravity * mass, 0.0f));
    }
}

void Sphere::update(float deltaTime) {
    if (dragged) {
        acceleration = glm::vec3(0.0f);
        velocity = glm::vec3(0.0f);
        return;
    }
    
    // Apply physics
    velocity += acceleration * deltaTime;
    
    // Apply drag
    velocity *= (1.0f - drag * deltaTime);
    
    // Update position
    position += velocity * deltaTime;
    
    // Reset acceleration for next frame
    acceleration = glm::vec3(0.0f);
    
    // Remove incorrect floor collision check - we handle this in the main loop now
}

void Sphere::render(unsigned int shaderProgram) {
    glBindVertexArray(VAO);
    
    // Set uniforms for sphere properties
    GLint colorLoc = glGetUniformLocation(shaderProgram, "sphereColor");
    glUniform3fv(colorLoc, 1, glm::value_ptr(sphereColor));
    
    // Draw sphere
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
} 
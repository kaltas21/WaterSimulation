#include "../include/GlassContainer.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

GlassContainer::GlassContainer(float width, float height, float depth)
    : width(width), height(height), depth(depth), position(0.0f),
      glassColor(0.8f, 0.8f, 0.9f), transparency(0.3f), refractionIndex(1.52f) {
}

GlassContainer::~GlassContainer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void GlassContainer::setDimensions(float width, float height, float depth) {
    this->width = width;
    this->height = height;
    this->depth = depth;
    generateMesh(); // Regenerate mesh with new dimensions
}

void GlassContainer::initialize() {
    generateMesh();
    
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

void GlassContainer::generateMesh() {
    vertices.clear();
    indices.clear();
    
    // Create box vertices
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    float halfDepth = depth / 2.0f;
    
    // Define the 8 corners of the box
    glm::vec3 corners[8] = {
        glm::vec3(-halfWidth, -halfHeight, -halfDepth), // 0: bottom left back
        glm::vec3(halfWidth, -halfHeight, -halfDepth),  // 1: bottom right back
        glm::vec3(halfWidth, halfHeight, -halfDepth),   // 2: top right back
        glm::vec3(-halfWidth, halfHeight, -halfDepth),  // 3: top left back
        glm::vec3(-halfWidth, -halfHeight, halfDepth),  // 4: bottom left front
        glm::vec3(halfWidth, -halfHeight, halfDepth),   // 5: bottom right front
        glm::vec3(halfWidth, halfHeight, halfDepth),    // 6: top right front
        glm::vec3(-halfWidth, halfHeight, halfDepth)    // 7: top left front
    };
    
    // Define the normals for each face
    glm::vec3 normals[6] = {
        glm::vec3(0.0f, 0.0f, -1.0f), // back
        glm::vec3(0.0f, 0.0f, 1.0f),  // front
        glm::vec3(1.0f, 0.0f, 0.0f),  // right
        glm::vec3(-1.0f, 0.0f, 0.0f), // left
        glm::vec3(0.0f, 1.0f, 0.0f),  // top
        glm::vec3(0.0f, -1.0f, 0.0f)  // bottom
    };
    
    // Define the vertex indices for each face (using the corners defined above)
    int faceIndices[6][4] = {
        {0, 1, 2, 3}, // back face
        {4, 5, 6, 7}, // front face
        {1, 5, 6, 2}, // right face
        {0, 4, 7, 3}, // left face
        {3, 2, 6, 7}, // top face - TODO: skip this face
        {0, 1, 5, 4}  // bottom face
    };
    
    // UV coordinates for each vertex of a face
    glm::vec2 uvs[4] = {
        glm::vec2(0.0f, 0.0f), // bottom left
        glm::vec2(1.0f, 0.0f), // bottom right
        glm::vec2(1.0f, 1.0f), // top right
        glm::vec2(0.0f, 1.0f)  // top left
    };
    
    // Create vertices for each face (skip top face to create open container)
    for (int i = 0; i < 6; i++) {
        // Skip the top face (index 4) to create an open container
        if (i == 4) continue;
        
        for (int j = 0; j < 4; j++) {
            // Position
            vertices.push_back(corners[faceIndices[i][j]].x);
            vertices.push_back(corners[faceIndices[i][j]].y);
            vertices.push_back(corners[faceIndices[i][j]].z);
            
            // Normal
            vertices.push_back(normals[i].x);
            vertices.push_back(normals[i].y);
            vertices.push_back(normals[i].z);
            
            // Texture coordinates
            vertices.push_back(uvs[j].x);
            vertices.push_back(uvs[j].y);
        }
        
        // Create indices for two triangles that make up this face
        int baseIndex = i < 4 ? i * 4 : (i - 1) * 4; // Adjust indices when skipping the top face
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
    }
}

void GlassContainer::render(unsigned int shaderProgram) {
    glBindVertexArray(VAO);
    
    // Set uniforms for glass properties
    GLint colorLoc = glGetUniformLocation(shaderProgram, "glassColor");
    GLint transparencyLoc = glGetUniformLocation(shaderProgram, "glassTransparency");
    GLint refractionIndexLoc = glGetUniformLocation(shaderProgram, "glassRefractionIndex");
    
    glUniform3fv(colorLoc, 1, glm::value_ptr(glassColor));
    glUniform1f(transparencyLoc, transparency);
    glUniform1f(refractionIndexLoc, refractionIndex);
    
    // Draw container
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
} 
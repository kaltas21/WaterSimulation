#include "../include/Skybox.h"
#include "../include/InitShader.h"
#include <iostream>
#include <filesystem>

// STB Image implementation - header only library
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

namespace WaterSim {

const float Skybox::skyboxVertices[] = {
    // positions   
    // back       
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    // top
    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    // bottom
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    // right
    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    // left
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,
    // front
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

Skybox::Skybox() : VAO(0), VBO(0), cubemapTexture(0), shaderProgram(0), loaded(false) {
}

Skybox::~Skybox() {
    cleanup();
}

void Skybox::initialize() {
    setupCube();
    setupShaders();
}

void Skybox::setupCube() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void Skybox::setupShaders() {
    shaderProgram = InitShader("shaders/skybox.vs", "shaders/skybox.fs");
    if (shaderProgram == 0) {
        std::cerr << "Failed to load skybox shaders!" << std::endl;
    }
}

void Skybox::loadCubemap(const std::vector<std::string>& faces) {
    if (faces.size() != 6) {
        std::cerr << "Skybox requires exactly 6 face textures!" << std::endl;
        return;
    }
    
    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    
    for (unsigned int i = 0; i < faces.size(); i++) {
        int width, height, nrChannels;
        
        // Check if file exists
        if (!std::filesystem::exists(faces[i])) {
            std::cerr << "Skybox texture file not found: " << faces[i] << std::endl;
            continue;
        }
        
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;
            
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
            std::cout << "Loaded skybox face: " << faces[i] << " (" << width << "x" << height << ")" << std::endl;
        } else {
            std::cerr << "Failed to load skybox texture: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    loaded = true;
}

void Skybox::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!loaded || shaderProgram == 0) {
        return;
    }
    
    // Change depth function so depth test passes when values are equal to depth buffer's content
    glDepthFunc(GL_LEQUAL);
    
    glUseProgram(shaderProgram);
    
    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Bind skybox texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "skybox"), 0);
    
    // Render skybox cube
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    // Set depth function back to default
    glDepthFunc(GL_LESS);
}

unsigned int Skybox::loadTexture(const std::string& path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
    }
    
    return textureID;
}

void Skybox::cleanup() {
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (cubemapTexture) {
        glDeleteTextures(1, &cubemapTexture);
        cubemapTexture = 0;
    }
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    loaded = false;
}

} // namespace WaterSim
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>

namespace WaterSim {

class Skybox {
public:
    Skybox();
    ~Skybox();

    void initialize();
    void loadCubemap(const std::vector<std::string>& faces);
    void render(const glm::mat4& view, const glm::mat4& projection);
    void cleanup();

    // Getters
    unsigned int getCubemapTexture() const { return cubemapTexture; }
    bool isLoaded() const { return loaded; }

private:
    // OpenGL resources
    unsigned int VAO, VBO;
    unsigned int cubemapTexture;
    unsigned int shaderProgram;
    
    // State
    bool loaded;
    
    // Helper methods
    void setupCube();
    void setupShaders();
    unsigned int loadTexture(const std::string& path);
    
    // Skybox cube vertices
    static const float skyboxVertices[];
};

} // namespace WaterSim
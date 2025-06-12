#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include "GLResources.h"

namespace WaterSim {

class ResourceManager {
private:
    std::unordered_map<std::string, GLShaderProgram> shaders;
    std::unordered_map<std::string, GLTexture> textures;
    
    static std::unique_ptr<ResourceManager> instance;
    
public:
    static ResourceManager& getInstance() {
        if (!instance) {
            instance = std::make_unique<ResourceManager>();
        }
        return *instance;
    }
    
    // Shader management
    GLShaderProgram& loadShader(const std::string& name, 
                                const std::string& vertexPath, 
                                const std::string& fragmentPath);
    
    GLShaderProgram& getShader(const std::string& name);
    
    // Texture management
    GLTexture& loadTexture(const std::string& name, const std::string& path);
    GLTexture& createTexture(const std::string& name);
    GLTexture& getTexture(const std::string& name);
    
    // Clean up all resources
    void clear();
    
private:
    ResourceManager() = default;
};

} // namespace WaterSim
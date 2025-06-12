#pragma once

#include <glad/glad.h>
#include <vector>
#include <cmath>
#include <random>

namespace WaterSim {

class TextureGenerator {
public:
    // Generate a dummy white texture
    static GLuint createDummyTexture();
    
    // Generate animated caustic texture
    static GLuint createCausticTexture(int size);
    
    // Generate tile texture for the pool floor
    static GLuint createTileTexture(int size, int tilesPerTexture = 16);
    
    // Generate skybox texture (placeholder)
    static GLuint createSkyboxTexture();
    
    // Generate steel/metal texture with reflective properties
    static GLuint createSteelTexture(int size);
    
private:
    // Helper to create texture from data
    static GLuint createTextureFromData(const std::vector<unsigned char>& data, 
                                       int width, int height, 
                                       GLenum format = GL_RGB,
                                       GLenum internalFormat = GL_RGB);
    
    // Perlin noise for caustic generation
    static float perlinNoise(float x, float y);
    static float fade(float t);
    static float lerp(float a, float b, float t);
    static float grad(int hash, float x, float y);
};

} // namespace WaterSim
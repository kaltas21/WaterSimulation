#pragma once

#include <glad/glad.h>
#include <string>

// Function to initialize and compile shaders
GLuint InitShader(const char* vertexShaderPath, const char* fragmentShaderPath);

// Function to initialize compute shader
GLuint InitComputeShader(const char* computeShaderPath);

// Helper function to read shader source from file
std::string ReadShaderSource(const char* filePath); 
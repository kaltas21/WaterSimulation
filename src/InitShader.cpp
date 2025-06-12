#include "../include/InitShader.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string ReadShaderSource(const char* filePath) {
    std::string content;
    std::ifstream fileStream(filePath, std::ios::in);
    
    if (!fileStream.is_open()) {
        // Try to look in the parent directory
        std::string parentPath = "../" + std::string(filePath);
        fileStream.open(parentPath, std::ios::in);
    }
    
    if (fileStream.is_open()) {
        std::stringstream sstr;
        sstr << fileStream.rdbuf();
        content = sstr.str();
        fileStream.close();
        std::cout << "Successfully loaded shader: " << filePath << " (" << content.length() << " chars)" << std::endl;
    }
    else {
        std::cerr << "ERROR: Could not open file " << filePath << std::endl;
        std::cerr << "Current working directory might be incorrect for shader loading." << std::endl;
    }
    
    return content;
}

GLuint InitShader(const char* vertexShaderPath, const char* fragmentShaderPath) {
    // Read shader source code
    std::string vertexShaderSrc = ReadShaderSource(vertexShaderPath);
    std::string fragmentShaderSrc = ReadShaderSource(fragmentShaderPath);
    
    // Check if shader sources were loaded successfully
    if (vertexShaderSrc.empty()) {
        std::cerr << "ERROR: Failed to read vertex shader source from: " << vertexShaderPath << std::endl;
        return 0;
    }
    if (fragmentShaderSrc.empty()) {
        std::cerr << "ERROR: Failed to read fragment shader source from: " << fragmentShaderPath << std::endl;
        return 0;
    }
    
    const char* vertexShaderCode = vertexShaderSrc.c_str();
    const char* fragmentShaderCode = fragmentShaderSrc.c_str();
    
    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
    glCompileShader(vertexShader);
    
    // Check for vertex shader compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Vertex shader compilation failed for: " << vertexShaderPath << std::endl;
        std::cerr << "Error details: " << infoLog << std::endl;
    } else {
        std::cout << "Vertex shader compiled successfully: " << vertexShaderPath << std::endl;
    }
    
    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragmentShader);
    
    // Check for fragment shader compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Fragment shader compilation failed for: " << fragmentShaderPath << std::endl;
        std::cerr << "Error details: " << infoLog << std::endl;
    } else {
        std::cout << "Fragment shader compiled successfully: " << fragmentShaderPath << std::endl;
    }
    
    // Create shader program and link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Check for shader program linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader program linking failed for: " << vertexShaderPath << " + " << fragmentShaderPath << std::endl;
        std::cerr << "Linking details: " << infoLog << std::endl;
        
        // Clean up and return 0 to indicate failure
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(shaderProgram);
        return 0;
    } else {
        std::cout << "Shader program linked successfully: " << vertexShaderPath << " + " << fragmentShaderPath << std::endl;
    }
    
    // Delete shaders as they're linked into the program now and no longer needed
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

GLuint InitComputeShader(const char* computeShaderPath) {
    // Read shader source code
    std::string computeShaderSrc = ReadShaderSource(computeShaderPath);
    const char* computeShaderCode = computeShaderSrc.c_str();
    
    if (computeShaderSrc.empty()) {
        std::cerr << "ERROR: Could not read compute shader source from " << computeShaderPath << std::endl;
        return 0;
    }
    
    // Create and compile compute shader
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &computeShaderCode, NULL);
    glCompileShader(computeShader);
    
    // Check for compute shader compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Compute shader compilation failed for " << computeShaderPath << "\n" << infoLog << std::endl;
        glDeleteShader(computeShader);
        return 0;
    }
    
    // Create shader program and link compute shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, computeShader);
    glLinkProgram(shaderProgram);
    
    // Check for shader program linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Compute shader program linking failed for " << computeShaderPath << "\n" << infoLog << std::endl;
        glDeleteShader(computeShader);
        glDeleteProgram(shaderProgram);
        return 0;
    }
    
    // Delete shader as it's linked into the program now and no longer needed
    glDeleteShader(computeShader);
    
    return shaderProgram;
} 
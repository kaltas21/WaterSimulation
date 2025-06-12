#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <stdexcept>
#include <string>

namespace WaterSim {

// RAII wrapper for OpenGL textures
class GLTexture {
private:
    GLuint id = 0;
    bool owned = false;

public:
    GLTexture() = default;
    
    GLTexture(GLTexture&& other) noexcept : id(other.id), owned(other.owned) {
        other.id = 0;
        other.owned = false;
    }
    
    GLTexture& operator=(GLTexture&& other) noexcept {
        if (this != &other) {
            cleanup();
            id = other.id;
            owned = other.owned;
            other.id = 0;
            other.owned = false;
        }
        return *this;
    }
    
    ~GLTexture() {
        cleanup();
    }
    
    void generate() {
        cleanup();
        glGenTextures(1, &id);
        owned = true;
    }
    
    void cleanup() {
        if (owned && id != 0) {
            glDeleteTextures(1, &id);
            id = 0;
            owned = false;
        }
    }
    
    GLuint get() const { return id; }
    GLuint getId() const { return id; }  // Alternative public getter
    operator GLuint() const { return id; }
    
    // Delete copy constructor and assignment
    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;
};

// RAII wrapper for OpenGL shader programs
class GLShaderProgram {
private:
    GLuint id = 0;

public:
    GLShaderProgram() = default;
    
    GLShaderProgram(GLShaderProgram&& other) noexcept : id(other.id) {
        other.id = 0;
    }
    
    GLShaderProgram& operator=(GLShaderProgram&& other) noexcept {
        if (this != &other) {
            cleanup();
            id = other.id;
            other.id = 0;
        }
        return *this;
    }
    
    ~GLShaderProgram() {
        cleanup();
    }
    
    void create() {
        cleanup();
        id = glCreateProgram();
        if (id == 0) {
            throw std::runtime_error("Failed to create shader program");
        }
    }
    
    // Set program ID directly (for compatibility with existing shader loading)
    void setId(GLuint programId) {
        cleanup();
        id = programId;
    }
    
    void cleanup() {
        if (id != 0) {
            glDeleteProgram(id);
            id = 0;
        }
    }
    
    void use() const {
        glUseProgram(id);
    }
    
    GLuint get() const { return id; }
    operator GLuint() const { return id; }
    
    // Check if the shader program is valid
    bool isValid() const { return id != 0; }
    
    // Uniform setters
    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(id, name.c_str()), value);
    }
    
    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(id, name.c_str()), value);
    }
    
    void setVec2(const std::string& name, const glm::vec2& value) const {
        glUniform2fv(glGetUniformLocation(id, name.c_str()), 1, &value[0]);
    }
    
    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(id, name.c_str()), 1, &value[0]);
    }
    
    void setMat3(const std::string& name, const glm::mat3& value) const {
        glUniformMatrix3fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }
    
    void setMat4(const std::string& name, const glm::mat4& value) const {
        glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }
    
    void setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(id, name.c_str()), value ? 1 : 0);
    }
    
    // Delete copy constructor and assignment
    GLShaderProgram(const GLShaderProgram&) = delete;
    GLShaderProgram& operator=(const GLShaderProgram&) = delete;
};

// RAII wrapper for OpenGL Vertex Array Objects
class GLVertexArray {
private:
    GLuint id = 0;

public:
    GLVertexArray() {
        glGenVertexArrays(1, &id);
    }
    
    GLVertexArray(GLVertexArray&& other) noexcept : id(other.id) {
        other.id = 0;
    }
    
    GLVertexArray& operator=(GLVertexArray&& other) noexcept {
        if (this != &other) {
            cleanup();
            id = other.id;
            other.id = 0;
        }
        return *this;
    }
    
    ~GLVertexArray() {
        cleanup();
    }
    
    void bind() const {
        glBindVertexArray(id);
    }
    
    void unbind() const {
        glBindVertexArray(0);
    }
    
    void cleanup() {
        if (id != 0) {
            glDeleteVertexArrays(1, &id);
            id = 0;
        }
    }
    
    GLuint get() const { return id; }
    
    // Delete copy constructor and assignment
    GLVertexArray(const GLVertexArray&) = delete;
    GLVertexArray& operator=(const GLVertexArray&) = delete;
};

// RAII wrapper for OpenGL buffers
class GLBuffer {
private:
    GLuint id = 0;
    GLenum type = GL_ARRAY_BUFFER;

public:
    GLBuffer() {
        glGenBuffers(1, &id);
    }
    
    GLBuffer(GLenum bufferType) : type(bufferType) {
        glGenBuffers(1, &id);
    }
    
    GLBuffer(GLBuffer&& other) noexcept : id(other.id), type(other.type) {
        other.id = 0;
    }
    
    GLBuffer& operator=(GLBuffer&& other) noexcept {
        if (this != &other) {
            cleanup();
            id = other.id;
            type = other.type;
            other.id = 0;
        }
        return *this;
    }
    
    ~GLBuffer() {
        cleanup();
    }
    
    void bind() const {
        glBindBuffer(type, id);
    }
    
    void unbind() const {
        glBindBuffer(type, 0);
    }
    
    void cleanup() {
        if (id != 0) {
            glDeleteBuffers(1, &id);
            id = 0;
        }
    }
    
    GLuint get() const { return id; }
    
    // Delete copy constructor and assignment
    GLBuffer(const GLBuffer&) = delete;
    GLBuffer& operator=(const GLBuffer&) = delete;
};

// RAII wrapper for OpenGL framebuffers
class GLFramebuffer {
private:
    GLuint id = 0;

public:
    GLFramebuffer() {
        glGenFramebuffers(1, &id);
    }
    
    GLFramebuffer(GLFramebuffer&& other) noexcept : id(other.id) {
        other.id = 0;
    }
    
    GLFramebuffer& operator=(GLFramebuffer&& other) noexcept {
        if (this != &other) {
            cleanup();
            id = other.id;
            other.id = 0;
        }
        return *this;
    }
    
    ~GLFramebuffer() {
        cleanup();
    }
    
    void bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, id);
    }
    
    void unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    bool isValid() const {
        if (id == 0) return false;
        glBindFramebuffer(GL_FRAMEBUFFER, id);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        return status == GL_FRAMEBUFFER_COMPLETE;
    }
    
    void cleanup() {
        if (id != 0) {
            glDeleteFramebuffers(1, &id);
            id = 0;
        }
    }
    
    GLuint get() const { return id; }
    
    // Delete copy constructor and assignment
    GLFramebuffer(const GLFramebuffer&) = delete;
    GLFramebuffer& operator=(const GLFramebuffer&) = delete;
};

// Additional texture types for SPH
class GLTexture2D : public GLTexture {
public:
    void bind() const {
        glBindTexture(GL_TEXTURE_2D, get());
    }
    
    void storage(GLsizei width, GLsizei height, GLenum internalFormat) {
        glBindTexture(GL_TEXTURE_2D, get());
        
        // Determine the correct format and type based on internal format
        GLenum format = GL_RGBA;
        GLenum type = GL_FLOAT;
        
        if (internalFormat == GL_DEPTH_COMPONENT32F || internalFormat == GL_DEPTH_COMPONENT32 || 
            internalFormat == GL_DEPTH_COMPONENT24 || internalFormat == GL_DEPTH_COMPONENT16) {
            format = GL_DEPTH_COMPONENT;
            type = GL_FLOAT;
        } else if (internalFormat == GL_R32F || internalFormat == GL_R16F || internalFormat == GL_R8) {
            format = GL_RED;
        } else if (internalFormat == GL_RG32F || internalFormat == GL_RG16F || internalFormat == GL_RG8) {
            format = GL_RG;
        } else if (internalFormat == GL_RGB32F || internalFormat == GL_RGB16F || internalFormat == GL_RGB8) {
            format = GL_RGB;
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
};

class GLTexture3D : public GLTexture {
public:
    void bind() const {
        glBindTexture(GL_TEXTURE_3D, get());
    }
    
    void storage(GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat) {
        glBindTexture(GL_TEXTURE_3D, get());
        
        // Determine the correct format based on internal format
        GLenum format = GL_RGBA;
        if (internalFormat == GL_R32F || internalFormat == GL_R16F || internalFormat == GL_R8) {
            format = GL_RED;
        } else if (internalFormat == GL_RG32F || internalFormat == GL_RG16F || internalFormat == GL_RG8) {
            format = GL_RG;
        } else if (internalFormat == GL_RGB32F || internalFormat == GL_RGB16F || internalFormat == GL_RGB8) {
            format = GL_RGB;
        }
        
        glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, width, height, depth, 0, format, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
};

// Transform feedback object wrapper
class GLTransformFeedback {
private:
    GLuint id = 0;

public:
    GLTransformFeedback() {
        glGenTransformFeedbacks(1, &id);
    }
    
    GLTransformFeedback(GLTransformFeedback&& other) noexcept : id(other.id) {
        other.id = 0;
    }
    
    GLTransformFeedback& operator=(GLTransformFeedback&& other) noexcept {
        if (this != &other) {
            cleanup();
            id = other.id;
            other.id = 0;
        }
        return *this;
    }
    
    ~GLTransformFeedback() {
        cleanup();
    }
    
    void bind() const {
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, id);
    }
    
    void unbind() const {
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
    }
    
    void cleanup() {
        if (id != 0) {
            glDeleteTransformFeedbacks(1, &id);
            id = 0;
        }
    }
    
    GLuint get() const { return id; }
    
    // Delete copy constructor and assignment
    GLTransformFeedback(const GLTransformFeedback&) = delete;
    GLTransformFeedback& operator=(const GLTransformFeedback&) = delete;
};

// Utility function to check OpenGL errors
inline void checkGLError(const std::string& location) {
#ifdef DEBUG
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        throw std::runtime_error("OpenGL error at " + location + ": " + std::to_string(error));
    }
#endif
}

} // namespace WaterSim
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

enum CameraMode {
    FREE_CAMERA,    // Normal FPS-style camera
    ORBIT_CAMERA    // Camera orbits around a fixed point
};

class Camera {
public:
    // Camera attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    
    // Euler angles
    float Yaw;
    float Pitch;
    
    // Camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    
    // Camera mode
    CameraMode Mode;
    glm::vec3 OrbitCenter;  // Center point for orbit mode
    float OrbitDistance;    // Distance from center in orbit mode
    
    // Constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f, float pitch = 0.0f);
    
    // Constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);
    
    // Returns the view matrix calculated using Euler angles and the LookAt matrix
    glm::mat4 GetViewMatrix() const;
    
    // Processes input received from keyboard
    void ProcessKeyboard(CameraMovement direction, float deltaTime);
    
    // Processes input received from a mouse input system
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    
    // Processes input received from a mouse scroll-wheel event
    void ProcessMouseScroll(float yoffset);
    
    // Set camera mode
    void SetMode(CameraMode mode);
    
    // Set orbit center (for orbit mode)
    void SetOrbitCenter(const glm::vec3& center);
    
    // Update orbit camera position based on angles and distance
    void UpdateOrbitPosition();
    
    // Set zoom/distance with specific value
    void SetZoom(float zoom);
    
    // Calculates the front vector from the Camera's Euler angles
    void updateCameraVectors();
};
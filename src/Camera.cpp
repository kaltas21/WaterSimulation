#include "../include/Camera.h"
#include <algorithm>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(2.5f), MouseSensitivity(0.1f), Zoom(45.0f),
      Mode(FREE_CAMERA), OrbitCenter(0.0f, 0.0f, 0.0f), OrbitDistance(10.0f) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(2.5f), MouseSensitivity(0.1f), Zoom(45.0f),
      Mode(FREE_CAMERA), OrbitCenter(0.0f, 0.0f, 0.0f), OrbitDistance(10.0f) {
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime) {
    if (Mode == ORBIT_CAMERA) {
        // In orbit mode, keyboard moves the orbit center
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            OrbitCenter += glm::vec3(Front.x, 0.0f, Front.z) * velocity;
        if (direction == BACKWARD)
            OrbitCenter -= glm::vec3(Front.x, 0.0f, Front.z) * velocity;
        if (direction == LEFT)
            OrbitCenter -= Right * velocity;
        if (direction == RIGHT)
            OrbitCenter += Right * velocity;
        if (direction == UP)
            OrbitCenter.y += velocity;
        if (direction == DOWN)
            OrbitCenter.y -= velocity;
        UpdateOrbitPosition();
    } else {
        // Free camera mode - normal movement
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        if (direction == UP)
            Position += Up * velocity;
        if (direction == DOWN)
            Position -= Up * velocity;
    }
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;
    
    Yaw += xoffset;
    Pitch += yoffset;
    
    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch) {
        Pitch = std::max(-89.0f, std::min(89.0f, Pitch));
    }
    
    // Update Front, Right and Up vectors using the updated Euler angles
    updateCameraVectors();
    
    // If in orbit mode, update position based on new angles
    if (Mode == ORBIT_CAMERA) {
        UpdateOrbitPosition();
    }
}

void Camera::ProcessMouseScroll(float yoffset) {
    if (Mode == ORBIT_CAMERA) {
        // In orbit mode, scroll changes distance from center
        OrbitDistance -= yoffset;
        OrbitDistance = std::max(1.0f, std::min(50.0f, OrbitDistance));
        UpdateOrbitPosition();
    } else {
        // In free mode, scroll changes FOV
        Zoom -= yoffset;
        Zoom = std::max(1.0f, std::min(120.0f, Zoom));  // Extended zoom range
    }
}

void Camera::updateCameraVectors() {
    // Calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    
    // Also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::SetMode(CameraMode mode) {
    Mode = mode;
    if (mode == ORBIT_CAMERA) {
        // Calculate initial orbit distance based on current position
        OrbitDistance = glm::length(Position - OrbitCenter);
        UpdateOrbitPosition();
    }
}

void Camera::SetOrbitCenter(const glm::vec3& center) {
    OrbitCenter = center;
    if (Mode == ORBIT_CAMERA) {
        UpdateOrbitPosition();
    }
}

void Camera::UpdateOrbitPosition() {
    // Calculate position based on spherical coordinates
    float x = OrbitDistance * cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
    float y = OrbitDistance * sin(glm::radians(Pitch));
    float z = OrbitDistance * cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
    
    Position = OrbitCenter + glm::vec3(x, y, z);
    
    // Look at the orbit center
    Front = glm::normalize(OrbitCenter - Position);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::SetZoom(float zoom) {
    if (Mode == ORBIT_CAMERA) {
        OrbitDistance = zoom;
        OrbitDistance = std::max(1.0f, std::min(50.0f, OrbitDistance));
        UpdateOrbitPosition();
    } else {
        Zoom = zoom;
        Zoom = std::max(1.0f, std::min(120.0f, Zoom));
    }
} 
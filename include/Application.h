#pragma once

#include <memory>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Config.h"
#include "GLResources.h"
#include "Camera.h"
#include "WaterSurface.h"
#include "Sphere.h"
#include "GlassContainer.h"

namespace WaterSim {

class Application {
private:
    // Configuration
    Config config;
    
    // Window
    GLFWwindow* window = nullptr;
    
    // Camera
    std::unique_ptr<Camera> camera;
    float lastX, lastY;
    bool firstMouse = true;
    
    // Scene objects
    std::unique_ptr<WaterSurface> waterSurface;
    std::unique_ptr<Sphere> sphere;
    std::unique_ptr<GlassContainer> glassContainer;
    
    // Shaders
    GLShaderProgram waterShader;
    GLShaderProgram sphereShader;
    GLShaderProgram glassShader;
    
    // Textures
    GLTexture skyboxTexture;
    GLTexture causticTexture;
    GLTexture tileTexture;
    
    // Timing
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float totalTime = 0.0f;
    
    // Input state
    bool isDraggingSphere = false;
    bool isRightMousePressed = false;
    glm::vec3 lastMouseWorldPos{0.0f};
    
    // Simulation state
    bool useGravity = false;
    bool enableMicroWaves = true;
    float waterHeight = 0.0f;
    
    // Ripple creation state
    bool isCreatingRipple = false;
    float rippleHoldTime = 0.0f;
    glm::vec3 ripplePosition{0.0f};
    
    // Static instance for callbacks
    static Application* instance;
    
public:
    Application();
    ~Application();
    
    // Main application lifecycle
    void run();
    
private:
    // Initialization
    void initializeGLFW();
    void createWindow();
    void initializeGLAD();
    void initializeImGui();
    void loadShaders();
    void createTextures();
    void createSceneObjects();
    
    // Main loop
    void updateDeltaTime();
    void processInput();
    void updateSimulation();
    void render();
    void renderUI();
    
    // Cleanup
    void cleanup();
    
    // Input callbacks
    void onFramebufferResize(int width, int height);
    void onMouseMove(double xpos, double ypos);
    void onMouseButton(int button, int action, int mods);
    void onScroll(double xoffset, double yoffset);
    
    // Helper methods
    glm::vec3 screenToWorld(double xpos, double ypos, float depth);
    void updateSpherePhysics();
    void checkSphereWaterInteraction();
    void createRippleAtMouse();
    
    // Static callback wrappers
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};

} // namespace WaterSim
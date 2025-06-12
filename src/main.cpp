#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <random>
#include <algorithm>

#include "../include/InitShader.h"
#include "../include/Camera.h"
#include "../include/WaterSurface.h"
#include "../include/Sphere.h"
#include "../include/GlassContainer.h"
#include "../include/Framebuffer.h"
#include "../include/HeightMapTexture.h"
#include "../include/ReflectionRenderer.h"
#include "../include/PostProcessManager.h"
#include "../include/RayTracingManager.h"
#include "../include/Config.h"
#include "../include/SimulationManager.h"
#include "../include/MainMenu.h"
#include "../include/Skybox.h"


// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, float deltaTime);
void renderUI(float deltaTime);
float calculateFPS(float deltaTime);
unsigned int loadSkybox(std::vector<std::string> faces);
unsigned int createDummyTexture();
unsigned int createCausticTexture(int size);
unsigned int createTileTexture(int size);
unsigned int createSteelTexture(int size);
void enableAnisotropicFiltering();
void renderScene(const Camera& camera, float waterLevel, bool isReflection, bool isRefraction);
void updateWaveSimulation(float deltaTime, float time);

// OpenGL debug callback
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                           GLsizei length, const GLchar* message, const void* userParam);

// Helper function to validate shader program before use
bool isShaderProgramValid(GLuint program) {
    if (program == 0) return false;
    
    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    return isLinked == GL_TRUE;
}

// Helper function to check OpenGL errors
void checkGLError(const char* operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error after " << operation << ": ";
        switch (error) {
            case GL_INVALID_ENUM: std::cerr << "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: std::cerr << "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: std::cerr << "GL_INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_OUT_OF_MEMORY: std::cerr << "GL_OUT_OF_MEMORY"; break;
            default: std::cerr << "Unknown error code: " << error; break;
        }
        std::cerr << std::endl;
    }
}

// Settings
unsigned int SCR_WIDTH = 1280;
unsigned int SCR_HEIGHT = 720;

// Container properties
const float FLOOR_LEVEL = -5.0f; // Define floor level as a constant

// Camera
Camera camera(glm::vec3(0.0f, 5.0f, 15.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float fpsCounter = 0.0f;

// Sphere dragging velocity tracking
glm::vec3 previousSpherePos(0.0f);
float dragVelocityTrackTime = 0.0f;
const float DRAG_VELOCITY_SAMPLE_TIME = 0.05f; // Sample every 50ms

// Configuration
WaterSim::Config config;

// Objects
Sphere* sphere = nullptr;
GlassContainer* container = nullptr;
WaterSim::Skybox* skybox = nullptr;

// Simulation management
WaterSim::SimulationManager* simulationManager = nullptr;
WaterSim::MainMenu* mainMenu = nullptr;

// Advanced rendering systems
ReflectionRenderer* reflectionRenderer = nullptr;
PostProcessManager* postProcessManager = nullptr;
HeightMapTexture* waveHeightMap = nullptr;
Framebuffer* mainSceneFBO = nullptr;
WaterSim::RayTracingManager* rayTracingManager = nullptr;

// Shader programs
GLuint waterShader = 0;
GLuint glassShader = 0;
GLuint sphereShader = 0;
GLuint foamShader = 0;

// Textures
GLuint skyboxTexture = 0;
GLuint causticTexture = 0;
GLuint tileTexture = 0;
GLuint steelTexture = 0;

// Mouse interaction
bool isDraggingSphere = false;
bool isRightMousePressed = false; // Track right mouse button state

// Sphere appearance settings (global so they can be shared)
bool enableSphereReflections = true;
float sphereReflectivity = 0.95f;
glm::vec3 lastMouseWorldPos(0.0f);

// Ray tracing state
bool rayTracingEnabled = false;
int rayTracingQuality = 0; // OFF by default


// Simulation settings
bool useGravity = false;
float gravity = 9.8f;

// Water simulation settings
bool enableMicroWaves = true;

// Add these global variables for ripple creation
bool isCreatingRipple = false;
float rippleHoldTime = 0.0f;
glm::vec3 ripplePosition(0.0f);
const float MAX_RIPPLE_MAGNITUDE = 0.5f;
const float RIPPLE_CHARGE_RATE = 0.5f; // How fast ripple magnitude increases per second

// SPH particle system settings
bool sprayParticles = false;
float particleEmissionRate = 50.0f;

int main() {
    // Initialize GLFW for Windows with maximum GPU utilization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8); // 8x MSAA
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE); // Enable debug context
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Water Simulation", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    
    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // Load OpenGL functions through GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // Print OpenGL info
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    
    // Enable OpenGL debug output for performance monitoring
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback((GLDEBUGPROC)debugCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        std::cout << "OpenGL Debug output enabled for performance monitoring" << std::endl;
    }
    
    // Clear any pending OpenGL errors
    while (glGetError() != GL_NO_ERROR) {
        // Clear error queue
    }
    std::cout << "OpenGL error queue cleared" << std::endl;
    
    // Print GPU information for optimization
    std::cout << "GPU Vendor: " << reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << std::endl;
    std::cout << "GPU Renderer: " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << std::endl;
    GLint maxComputeWorkGroups[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxComputeWorkGroups[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxComputeWorkGroups[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxComputeWorkGroups[2]);
    std::cout << "Max Compute Work Groups: " << maxComputeWorkGroups[0] << "x" << maxComputeWorkGroups[1] << "x" << maxComputeWorkGroups[2] << std::endl;
    
    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);  // Enable face culling for better transparency
    glEnable(GL_MULTISAMPLE); // Enable multisampling
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io; // Suppress unused variable warning
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer bindings for Windows OpenGL 4.6
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");
    
    // Initialize shaders
    std::cout << "Initializing shaders..." << std::endl;
    checkGLError("before shader initialization");
    
    waterShader = InitShader("shaders/water.vs", "shaders/water.fs");
    checkGLError("water shader initialization");
    
    glassShader = InitShader("shaders/glass.vs", "shaders/glass.fs");
    checkGLError("glass shader initialization");
    
    sphereShader = InitShader("shaders/sphere.vs", "shaders/sphere.fs");
    checkGLError("sphere shader initialization");
    
    foamShader = InitShader("shaders/foam.vs", "shaders/foam.fs");
    checkGLError("foam shader initialization");
    
    // Check if shaders were successfully created
    if (waterShader == 0) {
        std::cerr << "ERROR: Failed to create water shader program!" << std::endl;
        glfwTerminate();
        return -1;
    }
    if (glassShader == 0) {
        std::cerr << "ERROR: Failed to create glass shader program!" << std::endl;
        glfwTerminate();
        return -1;
    }
    if (sphereShader == 0) {
        std::cerr << "ERROR: Failed to create sphere shader program!" << std::endl;
        glfwTerminate();
        return -1;
    }
    if (foamShader == 0) {
        std::cerr << "ERROR: Failed to create foam shader program!" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    std::cout << "All main shaders initialized successfully." << std::endl;
    
    // Validate shader programs
    GLint validateStatus;
    GLchar infoLog[512];
    
    // Validate water shader
    glValidateProgram(waterShader);
    glGetProgramiv(waterShader, GL_VALIDATE_STATUS, &validateStatus);
    if (validateStatus != GL_TRUE) {
        glGetProgramInfoLog(waterShader, 512, NULL, infoLog);
        std::cerr << "Water shader validation failed: " << infoLog << std::endl;
    }
    
    // Validate glass shader
    glValidateProgram(glassShader);
    glGetProgramiv(glassShader, GL_VALIDATE_STATUS, &validateStatus);
    if (validateStatus != GL_TRUE) {
        glGetProgramInfoLog(glassShader, 512, NULL, infoLog);
        std::cerr << "Glass shader validation failed: " << infoLog << std::endl;
    }
    
    // Validate sphere shader
    glValidateProgram(sphereShader);
    glGetProgramiv(sphereShader, GL_VALIDATE_STATUS, &validateStatus);
    if (validateStatus != GL_TRUE) {
        glGetProgramInfoLog(sphereShader, 512, NULL, infoLog);
        std::cerr << "Sphere shader validation failed: " << infoLog << std::endl;
    }
    
    // Initialize skybox
    skybox = new WaterSim::Skybox();
    skybox->initialize();
    
    // Load skybox textures with proper paths and correct order
    std::vector<std::string> skyboxFaces = {
        "textures/skybox/px.png", // positive X (right)
        "textures/skybox/nx.png", // negative X (left)
        "textures/skybox/py.png", // positive Y (top)
        "textures/skybox/ny.png", // negative Y (bottom)
        "textures/skybox/pz.png", // positive Z (front)
        "textures/skybox/nz.png"  // negative Z (back)
    };
    skybox->loadCubemap(skyboxFaces);
    skyboxTexture = skybox->getCubemapTexture();
    
    // Generate a caustic texture for underwater lighting effects
    causticTexture = createCausticTexture(512);
    
    // Generate a tile texture for the pool
    tileTexture = createTileTexture(512);
    
    // Generate steel texture for the sphere
    steelTexture = createSteelTexture(512);
    
    // Initialize simulation objects
    sphere = new Sphere(1.0f);
    sphere->initialize();
    sphere->setPosition(glm::vec3(0.0f, 3.0f, 0.0f));
    sphere->setColor(glm::vec3(0.3f, 0.7f, 0.9f));
    sphere->setMass(2.0f);
    
    container = new GlassContainer(10.0f, 10.0f, 10.0f);
    container->initialize();
    
    // Initialize simulation manager and main menu
    simulationManager = new WaterSim::SimulationManager(config);
    mainMenu = new WaterSim::MainMenu();
    
    // Create advanced rendering systems
    reflectionRenderer = new ReflectionRenderer(SCR_WIDTH, SCR_HEIGHT);
    postProcessManager = new PostProcessManager(SCR_WIDTH, SCR_HEIGHT);
    rayTracingManager = new WaterSim::RayTracingManager(config);
    rayTracingManager->initialize(SCR_WIDTH, SCR_HEIGHT);
    waveHeightMap = new HeightMapTexture(256, 256);
    mainSceneFBO = new Framebuffer(SCR_WIDTH, SCR_HEIGHT, FRAMEBUFFER_COLOR_DEPTH);
    
    // Initialize simulation parameters
    simulationManager->setWaterHeight(0.0f);
    
    // Create water volume geometry for inside the container
    GLuint waterVolumeVAO, waterVolumeVBO, waterVolumeEBO;
    glGenVertexArrays(1, &waterVolumeVAO);
    glGenBuffers(1, &waterVolumeVBO);
    glGenBuffers(1, &waterVolumeEBO);
    
    // Water volume vertices - simple box
    float waterHalfWidth = 5.0f - 0.05f; // Slightly smaller than container
    float waterDepth = 5.0f - 0.05f; // Slightly smaller than container
    std::vector<float> waterVolumeVertices = {
        // Positions           // Normals           // TexCoords
        // Bottom face (floor level is at -5.0f)
        -waterHalfWidth, FLOOR_LEVEL, -waterDepth,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         waterHalfWidth, FLOOR_LEVEL, -waterDepth,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         waterHalfWidth, FLOOR_LEVEL,  waterDepth,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -waterHalfWidth, FLOOR_LEVEL,  waterDepth,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        
        // Top face (will be adjusted to water height in render loop)
        -waterHalfWidth,  0.0f, -waterDepth,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         waterHalfWidth,  0.0f, -waterDepth,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         waterHalfWidth,  0.0f,  waterDepth,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -waterHalfWidth,  0.0f,  waterDepth,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
    };
    
    // The y-coordinates will be adjusted based on water height in the render loop
    std::vector<unsigned int> waterVolumeIndices = {
        0, 1, 5, 0, 5, 4, // Back face
        1, 2, 6, 1, 6, 5, // Right face
        2, 3, 7, 2, 7, 6, // Front face
        3, 0, 4, 3, 4, 7, // Left face
        4, 5, 6, 4, 6, 7, // Top face
        0, 1, 2, 0, 2, 3  // Bottom face
    };
    
    glBindVertexArray(waterVolumeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waterVolumeVBO);
    glBufferData(GL_ARRAY_BUFFER, waterVolumeVertices.size() * sizeof(float), waterVolumeVertices.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterVolumeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterVolumeIndices.size() * sizeof(unsigned int), waterVolumeIndices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Calculate FPS
        float fps = calculateFPS(deltaTime);
        
        // Process input
        processInput(window, deltaTime);
        
        // Update physics and objects
        sphere->setUseGravity(useGravity);
        if (useGravity) {
            sphere->applyGravity(gravity);
        }
        sphere->update(deltaTime);
        
        // Handle menu interactions and simulation selection
        if (mainMenu->hasSelectionChanged()) {
            WaterSim::SimulationType selectedType = mainMenu->getSelectedSimulation();
            simulationManager->setSimulationType(selectedType);
            mainMenu->clearSelectionChanged();
        }
        
        // Check sphere collision with water and create interactions
        glm::vec3 spherePos = sphere->getPosition();
        float sphereRadius = sphere->getRadius();
        
        // Different collision detection based on simulation type
        bool isBelowWater = false;
        static bool wasBelowWater = false;
        
        if (simulationManager->isRegularWaterActive()) {
            float waterHeight = simulationManager->getWaterHeight();
            isBelowWater = spherePos.y - sphereRadius <= waterHeight;
        } else if (simulationManager->isSPHComputeActive()) {
            // For SPH, check if sphere is in the container bounds where particles exist
            float containerBottom = -4.5f;  // SPH particle container bottom
            float containerTop = -1.0f;     // SPH particle container top
            isBelowWater = (spherePos.y - sphereRadius <= containerTop && spherePos.y + sphereRadius >= containerBottom);
        }
        
        if (isBelowWater && !wasBelowWater && sphere->getVelocity().y < -0.5f) {
            // Create interaction based on simulation type
            float interactionMagnitude = std::abs(sphere->getVelocity().y);
            
            if (simulationManager->isRegularWaterActive()) {
                simulationManager->createSplash(spherePos, interactionMagnitude);
            } else if (simulationManager->isSPHComputeActive()) {
                // Enhanced collision for SPH particles
                glm::vec3 impulse = sphere->getVelocity() * 10.0f;  // Strong impulse for visible effect
                float impulseRadius = sphereRadius * 4.0f;  // Large interaction radius
                simulationManager->applyImpulse(spherePos, impulse, impulseRadius);
                
                std::cout << "SPH Sphere collision! Pos: (" << spherePos.x << ", " << spherePos.y << ", " << spherePos.z 
                          << ") Impulse magnitude: " << glm::length(impulse) << std::endl;
            }
        }
        wasBelowWater = isBelowWater;
        
        // Apply physics and interaction when sphere is in water
        if (isBelowWater) {
            glm::vec3 velocity = sphere->getVelocity();
            
            if (simulationManager->isRegularWaterActive()) {
                // Regular water physics
                float waterHeight = simulationManager->getWaterHeight();
                float submergedDepth = std::min(waterHeight - (spherePos.y - sphereRadius), 2.0f * sphereRadius);
                float submergedRatio = submergedDepth / (2.0f * sphereRadius);
                float dragFactor = 2.0f * submergedRatio;
                sphere->applyForce(-velocity * dragFactor);
                
                // Add continuous water flow when sphere is moving underwater
                float lateralSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
                if (lateralSpeed > 0.2f && submergedRatio > 0.5f) {
                    glm::vec2 lateralVelocity(velocity.x, velocity.z);
                    simulationManager->addWaterFlowImpulse(spherePos, lateralVelocity * 0.3f, sphereRadius * 1.5f);
                }
            } else if (simulationManager->isSPHComputeActive()) {
                // SPH particle physics
                float dragFactor = 1.0f;  // SPH particle resistance
                sphere->applyForce(-velocity * dragFactor);
                
                // Use SPH gravity instead of regular gravity
                glm::vec3 sphGravity(0.0f, -9.81f, 0.0f);  // Use SPH physics gravity
                sphere->applyForce(sphGravity * sphere->getMass());
                
                // Continuous particle interaction
                if (glm::length(velocity) > 0.1f) {
                    static int frameCounter = 0;
                    if (frameCounter++ % 5 == 0) {  // More frequent interaction
                        glm::vec3 continuousImpulse = velocity * 2.0f;
                        simulationManager->applyImpulse(spherePos, continuousImpulse, sphereRadius * 3.0f);
                    }
                }
            }
        }
        // ÖNEMLİ
        // HATIRLA: The actual floor is at y=-5, not y=0 
        // Check collision with the container floor
        if (spherePos.y - sphereRadius <= FLOOR_LEVEL + 0.001f) {
            // Stop vertical movement and apply a small bounce
            glm::vec3 vel = sphere->getVelocity();
            if (vel.y < 0) {
                // Bounce with reduced energy
                vel.y = -vel.y * 0.3f; // 30% of energy preserved in bounce
                
                // If velocity is very low, just stop
                if (std::abs(vel.y) < 0.1f) {
                    vel.y = 0;
                }
                
                // Reset position to be exactly on the floor
                spherePos.y = FLOOR_LEVEL + sphereRadius + 0.001f;
                sphere->setPosition(spherePos);
                sphere->setVelocity(vel);
            }
        }
        
        // Update simulation manager
        simulationManager->update(deltaTime);
        
        // Handle simulation-specific interactions
        if (simulationManager->isSPHComputeActive() && sprayParticles) {
            glm::vec3 streamOrigin(0.0f, 3.0f, 0.0f);
            glm::vec3 streamDirection(0.0f, -1.0f, 0.1f);
            simulationManager->addFluidStream(streamOrigin, streamDirection, particleEmissionRate * deltaTime);
        }
        
        // === ADVANCED RENDERING PIPELINE ===
        
        // 1. UPDATE GPU WAVE SIMULATION (if compute shader available)
        updateWaveSimulation(deltaTime, currentFrame);
        
        // Get water height from simulation manager for rendering
        float currentWaterHeight = simulationManager->getWaterHeight();
        
        // Only do reflection/refraction passes for regular water surface
        if (simulationManager->isRegularWaterActive()) {
            // 2. RENDER REFLECTION PASS
            reflectionRenderer->beginReflectionRender(camera, currentWaterHeight);
            renderScene(camera, currentWaterHeight, true, false); // Render scene for reflection
            reflectionRenderer->endReflectionRender();
            
            // 3. RENDER REFRACTION PASS  
            reflectionRenderer->beginRefractionRender(camera, currentWaterHeight);
            renderScene(camera, currentWaterHeight, false, true); // Render scene for refraction
            reflectionRenderer->endRefractionRender();
        }
        
        // 4. RENDER MAIN SCENE TO FRAMEBUFFER
        mainSceneFBO->bind();
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Create transformation matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        
        // Render skybox first (early z-test rejection)
        if (skybox && skybox->isLoaded()) {
            skybox->render(view, projection);
        }
        
        // First render the sphere
        if (isShaderProgramValid(sphereShader)) {
            glUseProgram(sphereShader);
            
            // Set sphere shader uniforms
            glUniformMatrix4fv(glGetUniformLocation(sphereShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(sphereShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, sphere->getPosition());
            glUniformMatrix4fv(glGetUniformLocation(sphereShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            
            // Set lighting uniforms
            glUniform3f(glGetUniformLocation(sphereShader, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
            glUniform3f(glGetUniformLocation(sphereShader, "lightPos"), 5.0f, 10.0f, 5.0f);
            glUniform3f(glGetUniformLocation(sphereShader, "lightColor"), 1.0f, 1.0f, 1.0f);
            glUniform1f(glGetUniformLocation(sphereShader, "ambientStrength"), 0.1f);
            glUniform1f(glGetUniformLocation(sphereShader, "specularStrength"), 0.8f); // Moderate for realistic metal
            glUniform1f(glGetUniformLocation(sphereShader, "shininess"), 128.0f); // High but not excessive for metal
            
            // Enable texture for steel appearance
            glUniform1i(glGetUniformLocation(sphereShader, "useTexture"), 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, steelTexture);
            glUniform1i(glGetUniformLocation(sphereShader, "sphereTexture"), 0);
            
            // Enable reflections for mirror-like appearance
            glUniform1i(glGetUniformLocation(sphereShader, "enableReflections"), enableSphereReflections ? 1 : 0);
            glUniform1f(glGetUniformLocation(sphereShader, "reflectivity"), sphereReflectivity);
            
            // Bind skybox for environment reflections
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
            glUniform1i(glGetUniformLocation(sphereShader, "skybox"), 1);
            
            // Render sphere
            sphere->render(sphereShader);
        }
        
        // Render water simulation through simulation manager
        if (simulationManager->getCurrentType() != WaterSim::SimulationType::NONE) {
            // Only set up water shader for regular water surface
            if (simulationManager->isRegularWaterActive() && isShaderProgramValid(waterShader)) {
                // Set common shader uniforms for water rendering
                glUseProgram(waterShader);
                
                // Set lighting uniforms
                glUniform3f(glGetUniformLocation(waterShader, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
                glUniform3f(glGetUniformLocation(waterShader, "lightPos"), 5.0f, 10.0f, 5.0f);
                glUniform3f(glGetUniformLocation(waterShader, "lightColor"), 1.0f, 1.0f, 1.0f);
                glUniform1f(glGetUniformLocation(waterShader, "ambientStrength"), 0.1f);
                glUniform1f(glGetUniformLocation(waterShader, "specularStrength"), 0.5f);
                glUniform1f(glGetUniformLocation(waterShader, "shininess"), 64.0f);
                
                // Set time for water animation and caustics
                glUniform1f(glGetUniformLocation(waterShader, "time"), currentFrame);
                // Check if any waves have non-zero amplitude
                bool hasActiveWaves = false;
                WaterSurface* waterSurface = simulationManager->getWaterSurface();
                if (waterSurface) {
                    for (const auto& wave : waterSurface->getWaves()) {
                        if (std::abs(wave.amplitude) > 0.001f) {
                            hasActiveWaves = true;
                            break;
                        }
                    }
                    
                    // Set water color and transparency
                    glm::vec3 waterColor = waterSurface->getColor();
                    float transparency = waterSurface->getTransparency();
                    glUniform3f(glGetUniformLocation(waterShader, "waterColor"), waterColor.r, waterColor.g, waterColor.b);
                    glUniform1f(glGetUniformLocation(waterShader, "transparency"), transparency);
                }
                
                // Only enable micro-waves if we have active waves or if explicitly enabled
                bool shouldEnableMicroWaves = hasActiveWaves && enableMicroWaves;
                glUniform1i(glGetUniformLocation(waterShader, "enableMicroWaves"), shouldEnableMicroWaves);
                
                // Set skybox texture
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
                glUniform1i(glGetUniformLocation(waterShader, "skybox"), 0);
                
                // Set reflection and refraction textures
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, reflectionRenderer->getReflectionTexture());
                glUniform1i(glGetUniformLocation(waterShader, "reflectionTexture"), 1);
                
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, reflectionRenderer->getRefractionTexture());
                glUniform1i(glGetUniformLocation(waterShader, "refractionTexture"), 2);
                
                // Bind caustic texture
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, causticTexture);
                glUniform1i(glGetUniformLocation(waterShader, "causticTex"), 3);
                
                // Bind tile texture
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, tileTexture);
                glUniform1i(glGetUniformLocation(waterShader, "tileTexture"), 4);
                
                // Bind wave height map texture
                glActiveTexture(GL_TEXTURE5);
                waveHeightMap->bind(5);
                glUniform1i(glGetUniformLocation(waterShader, "waveHeightMap"), 5);
                
                // Render through simulation manager for regular water
                simulationManager->render(view, projection, waterShader, rayTracingEnabled);
                
                // Render foam particles if regular water is active
                if (isShaderProgramValid(foamShader)) {
                    WaterSurface* waterSurface = simulationManager->getWaterSurface();
                    if (waterSurface) {
                        waterSurface->renderFoam(foamShader, view, projection);
                    }
                }
            } else if (simulationManager->isSPHComputeActive()) {
                // Render SPH particles with their own rendering pipeline
                simulationManager->render(view, projection, 0, false);
            }
        }
        
        // Ray tracing integration for regular water surface
        if (rayTracingEnabled && rayTracingManager && simulationManager->isRegularWaterActive()) {
            WaterSurface* waterSurface = simulationManager->getWaterSurface();
            if (waterSurface) {
                // Get water surface geometry data for ray tracing
                GLuint waterVAO = waterSurface->getVAO();
                int waterVertexCount = waterSurface->getVertexCount();
                
                // Set water geometry for G-buffer rendering
                rayTracingManager->setWaterGeometry(waterVAO, waterVertexCount);
                
                // Perform ray traced water rendering
                glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
                rayTracingManager->renderWaterRayTraced(view, projection, camera.Position, lightPos);
                
                // Restore OpenGL state after ray tracing
                glBindFramebuffer(GL_FRAMEBUFFER, 0); // Ensure main framebuffer is bound
                glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // Restore viewport
                glUseProgram(0); // Clear shader program
                glBindVertexArray(0); // Clear VAO binding
                
                // Restore depth and blending state for normal rendering
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glDepthMask(GL_TRUE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
                // Reset texture bindings that might be affected by compute shaders
                for (int i = 0; i < 8; ++i) {
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                }
                glActiveTexture(GL_TEXTURE0); // Reset to texture unit 0
                
                // Blend ray tracing effects over the existing scene
                GLuint rayTracedTexture = rayTracingManager->getRayTracedTexture();
                if (rayTracedTexture != 0) {
                    // Enable alpha blending for ray traced effects
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Alpha blending
                    glDisable(GL_DEPTH_TEST);
                    
                    // Apply ray traced effects as enhancement over existing scene
                    postProcessManager->applyPostProcessing(rayTracedTexture);
                    
                    glDisable(GL_BLEND);
                    glEnable(GL_DEPTH_TEST);
                    
                    static int debugFrame = 0;
                    debugFrame++;
                    if (debugFrame == 1 || debugFrame % 60 == 0) {
                        std::cout << "Ray traced water enhanced: " << rayTracedTexture << std::endl;
                    }
                }
            }
        }
        
        // Disable depth writing for transparent glass
        glDepthMask(GL_FALSE);
        
        // Enable proper culling and blending for the glass
        glDisable(GL_CULL_FACE);  // Don't cull faces for glass to see both sides
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Standard transparency blending
        
        // Render the glass container last
        if (isShaderProgramValid(glassShader)) {
            // Debug output for glass rendering when ray tracing is enabled
            static int frameCount = 0;
            frameCount++;
            if (rayTracingEnabled && (frameCount == 1 || frameCount % 60 == 0)) {
                std::cout << "Rendering glass container with ray tracing enabled (frame " << frameCount << ")" << std::endl;
            }
            
            glUseProgram(glassShader);
            
            // Set glass shader uniforms
            glUniformMatrix4fv(glGetUniformLocation(glassShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(glassShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(glassShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            
            // Set skybox texture for glass shader
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
            glUniform1i(glGetUniformLocation(glassShader, "skybox"), 0);
            
            // Set lighting uniforms
            glUniform3f(glGetUniformLocation(glassShader, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
            glUniform3f(glGetUniformLocation(glassShader, "lightPos"), 5.0f, 10.0f, 5.0f);
            glUniform3f(glGetUniformLocation(glassShader, "lightColor"), 1.0f, 1.0f, 1.0f);
            glUniform1f(glGetUniformLocation(glassShader, "ambientStrength"), 0.2f);
            glUniform1f(glGetUniformLocation(glassShader, "specularStrength"), 0.5f);
            glUniform1f(glGetUniformLocation(glassShader, "shininess"), 32.0f);
            
            // Set glass properties - increase transparency
            glUniform1f(glGetUniformLocation(glassShader, "glassTransparency"), 0.15f); // More transparent
            glUniform3f(glGetUniformLocation(glassShader, "glassColor"), 0.95f, 0.95f, 1.0f); // Slightly bluer
            glUniform1f(glGetUniformLocation(glassShader, "glassRefractionIndex"), 1.05f); // Less refraction
            
            // Render container
            container->render(glassShader);
        }
        
        // Re-enable depth writing and culling for solid objects
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        
        // Render water volume only if regular water is active
        if (simulationManager->isRegularWaterActive() && isShaderProgramValid(waterShader)) {
            glUseProgram(waterShader);
            glBindVertexArray(waterVolumeVAO);
            
            // Update water volume top vertices based on current water height
            std::vector<float> updatedVertices = waterVolumeVertices;
            // Set Y position for top vertices (indices 4-7)
            float waterHeight = simulationManager->getWaterHeight();
            for (int i = 4; i < 8; i++) {
                updatedVertices[i * 8 + 1] = waterHeight; // Y coordinate
            }
            
            // Update the VBO
            glBindBuffer(GL_ARRAY_BUFFER, waterVolumeVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, updatedVertices.size() * sizeof(float), updatedVertices.data());
            
            // Apply same uniforms as water surface
            glUniformMatrix4fv(glGetUniformLocation(waterShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(waterShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            
            glm::mat4 model = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(waterShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            
            // Set skybox texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
            glUniform1i(glGetUniformLocation(waterShader, "skybox"), 0);
            
            // Check if any waves have non-zero amplitude for volume rendering
            bool hasActiveWavesForVolume = false;
            WaterSurface* waterSurface = simulationManager->getWaterSurface();
            if (waterSurface) {
                for (const auto& wave : waterSurface->getWaves()) {
                    if (std::abs(wave.amplitude) > 0.001f) {
                        hasActiveWavesForVolume = true;
                        break;
                    }
                }
            }
            
            // Only enable micro-waves if we have active waves or if explicitly enabled
            bool shouldEnableMicroWavesForVolume = hasActiveWavesForVolume && enableMicroWaves;
            glUniform1i(glGetUniformLocation(waterShader, "enableMicroWaves"), shouldEnableMicroWavesForVolume);
            
            // Set water properties - make water volume more visible but still transparent
            glm::vec3 waterColor(0.05f, 0.3f, 0.5f); // Default water color
            float transparency = 0.9f; // Default transparency
            if (waterSurface) {
                waterColor = waterSurface->getColor();
                transparency = waterSurface->getTransparency();
            }
            glm::vec3 volumeColor = waterColor * 0.9f; // Slightly less saturated for better transparency
            float volumeTransparency = std::min(transparency * 2.0f, 0.95f); // Higher transparency
            
            glUniform3f(glGetUniformLocation(waterShader, "waterColor"), volumeColor.r, volumeColor.g, volumeColor.b);
            glUniform1f(glGetUniformLocation(waterShader, "transparency"), volumeTransparency);
            
            // Set additional lighting parameters for crystal clear water
            glUniform1f(glGetUniformLocation(waterShader, "ambientStrength"), 0.2f); // Lower ambient for clearer water
            glUniform1f(glGetUniformLocation(waterShader, "specularStrength"), 0.4f); // Moderate specular for realistic water
            
            // Add time for water animation and caustics
            glUniform1f(glGetUniformLocation(waterShader, "time"), currentFrame);
            
            // Bind caustic texture
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, causticTexture);
            glUniform1i(glGetUniformLocation(waterShader, "causticTex"), 3);
            
            // Bind tile texture
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, tileTexture);
            glUniform1i(glGetUniformLocation(waterShader, "tileTexture"), 4);
            
            // Draw the water volume
            glDrawElements(GL_TRIANGLES, waterVolumeIndices.size(), GL_UNSIGNED_INT, 0);
            
            glBindVertexArray(0);
        }
        
        // Note: Sphere and container are already rendered above, no need to render again
        
        // Unbind main scene framebuffer
        mainSceneFBO->unbind();
        
        // 5. COMPOSITE RAY TRACED RESULTS
        if (rayTracingEnabled && rayTracingManager) {
            // The ray traced results are already available in the ray tracing manager
            // They will be composited during post-processing or directly rendered
        }
        
        // 6. APPLY POST-PROCESSING EFFECTS
        // Note: Don't clear here! We want to render the post-processed scene
        
        // Apply post-processing to the main scene
        postProcessManager->applyPostProcessing(
            mainSceneFBO->getColorTexture(),
            mainSceneFBO->getDepthTexture()
        );
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render ImGui UI
        mainMenu->render();
        renderUI(deltaTime);
        
        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    delete sphere;
    delete container;
    delete skybox;
    delete simulationManager;
    delete mainMenu;
    delete reflectionRenderer;
    delete postProcessManager;
    delete rayTracingManager;
    delete waveHeightMap;
    delete mainSceneFBO;
    
    // Cleanup water volume
    glDeleteVertexArrays(1, &waterVolumeVAO);
    glDeleteBuffers(1, &waterVolumeVBO);
    glDeleteBuffers(1, &waterVolumeEBO);
    
    // Cleanup shaders
    if (waterShader) glDeleteProgram(waterShader);
    if (glassShader) glDeleteProgram(glassShader);
    if (sphereShader) glDeleteProgram(sphereShader);
    if (foamShader) glDeleteProgram(foamShader);
    
    // Cleanup textures
    if (skyboxTexture) glDeleteTextures(1, &skyboxTexture);
    if (causticTexture) glDeleteTextures(1, &causticTexture);
    if (tileTexture) glDeleteTextures(1, &tileTexture);
    if (steelTexture) glDeleteTextures(1, &steelTexture);
    
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, float deltaTime) {
    // Toggle main menu with ESC key
    static bool escKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!escKeyPressed) {
            mainMenu->setMenuActive(!mainMenu->isMenuActive());
            escKeyPressed = true;
        }
    } else {
        escKeyPressed = false;
    }
    
    // Camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
    
    // Update ripple creation if holding mouse button
    if (isCreatingRipple) {
        rippleHoldTime += deltaTime;
        // Cap ripple magnitude
        rippleHoldTime = std::min(rippleHoldTime, MAX_RIPPLE_MAGNITUDE / RIPPLE_CHARGE_RATE);
    }
    
    // Toggle gravity with G key
    static bool gKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        if (!gKeyPressed) {
            useGravity = !useGravity;
            gKeyPressed = true;
        }
    } else {
        gKeyPressed = false;
    }
    
    // Add waves/ripples with R key
    static bool rKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!rKeyPressed) {
            WaterSurface::WaveParam newWave;
            newWave.direction = glm::normalize(glm::vec2(std::rand() / (float)RAND_MAX * 2.0f - 1.0f, 
                                                         std::rand() / (float)RAND_MAX * 2.0f - 1.0f));
            newWave.amplitude = 0.1f + std::rand() / (float)RAND_MAX * 0.2f;
            newWave.wavelength = 2.0f + std::rand() / (float)RAND_MAX * 3.0f;
            newWave.speed = 0.5f + std::rand() / (float)RAND_MAX;
            newWave.steepness = 0.3f + std::rand() / (float)RAND_MAX * 0.3f;
            
            if (simulationManager->isRegularWaterActive()) {
                WaterSurface* waterSurface = simulationManager->getWaterSurface();
                if (waterSurface) {
                    waterSurface->addWave(newWave);
                }
            }
            rKeyPressed = true;
        }
    } else {
        rKeyPressed = false;
    }
    
    // Clear waves with C key
    static bool cKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!cKeyPressed) {
            if (simulationManager->isRegularWaterActive()) {
                WaterSurface* waterSurface = simulationManager->getWaterSurface();
                if (waterSurface) {
                    waterSurface->clearWaves();
                    // Add default wave
                    WaterSurface::WaveParam defaultWave;
                    defaultWave.direction = glm::normalize(glm::vec2(1.0f, 1.0f));
                    defaultWave.amplitude = 0.1f;
                    defaultWave.wavelength = 4.0f;
                    defaultWave.speed = 1.0f;
                    defaultWave.steepness = 0.5f;
                    waterSurface->addWave(defaultWave);
                }
            }
            
            cKeyPressed = true;
        }
    } else {
        cKeyPressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Ignore minimized windows
    if (width == 0 || height == 0) return;
    
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
    
    // Update all framebuffers that depend on window size
    if (reflectionRenderer) {
        delete reflectionRenderer;
        reflectionRenderer = new ReflectionRenderer(width, height);
    }
    
    if (postProcessManager) {
        delete postProcessManager;
        postProcessManager = new PostProcessManager(width, height);
    }
    
    if (mainSceneFBO) {
        delete mainSceneFBO;
        mainSceneFBO = new Framebuffer(width, height, FRAMEBUFFER_COLOR_DEPTH);
    }
    
    if (rayTracingManager) {
        rayTracingManager->resize(width, height);
    }
    
    // Update camera aspect ratio
    lastX = width / 2.0f;
    lastY = height / 2.0f;
}

// Function to convert screen coordinates to world coordinates
glm::vec3 screenToWorld(float screenX, float screenY, float depth) {
    // Normalize screen coordinates to NDC
    float x = (2.0f * screenX) / SCR_WIDTH - 1.0f;
    float y = 1.0f - (2.0f * screenY) / SCR_HEIGHT;
    
    // Create NDC coordinates
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);
    
    // Convert to eye space
    glm::mat4 invProj = glm::inverse(glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f));
    glm::vec4 rayEye = invProj * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    
    // Convert to world space
    glm::mat4 invView = glm::inverse(camera.GetViewMatrix());
    glm::vec4 rayWorld = invView * rayEye;
    glm::vec3 rayDirection = glm::normalize(glm::vec3(rayWorld));
    
    // Calculate point at given depth from camera
    return camera.Position + rayDirection * depth;
} 

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    
    // Check if ImGui is using the mouse
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
    
    lastX = xpos;
    lastY = ypos;
    
    // Check if dragging the sphere
    if (isDraggingSphere) {
        // Convert mouse position to world coordinates at a fixed depth
        float depth = glm::length(camera.Position - sphere->getPosition());
        glm::vec3 worldPos = screenToWorld(xpos, ypos, depth);
        
        // Calculate movement
        glm::vec3 movement = worldPos - lastMouseWorldPos;
        
        // Store previous position for velocity calculation
        glm::vec3 oldPos = sphere->getPosition();
        
        // Update sphere position
        glm::vec3 newPos = sphere->getPosition() + movement;
        
        // Constrain within container bounds
        float containerHalfWidth = container->getWidth() / 2.0f - sphere->getRadius();
        float containerHalfDepth = container->getDepth() / 2.0f - sphere->getRadius();
        float containerHeight = container->getHeight() - sphere->getRadius();
        
        newPos.x = std::max(-containerHalfWidth, std::min(newPos.x, containerHalfWidth));
        newPos.y = std::max(FLOOR_LEVEL + sphere->getRadius(), std::min(newPos.y, containerHeight));
        newPos.z = std::max(-containerHalfDepth, std::min(newPos.z, containerHalfDepth));
        
        sphere->setPosition(newPos);
        
        // Calculate instantaneous velocity for ripple generation
        glm::vec3 instantVelocity = (newPos - oldPos) / deltaTime;
        
        // Update velocity tracking for release
        dragVelocityTrackTime += deltaTime;
        if (dragVelocityTrackTime > DRAG_VELOCITY_SAMPLE_TIME) {
            previousSpherePos = newPos;
            dragVelocityTrackTime = 0.0f;
        }
        
        // Check if sphere is near water surface and moving laterally
        float waterHeight = simulationManager->getWaterHeight();
        float sphereRadius = sphere->getRadius();
        bool nearWaterSurface = std::abs(sphere->getPosition().y - sphereRadius - waterHeight) < sphereRadius * 1.5f;
        
        if (nearWaterSurface && simulationManager->isRegularWaterActive()) {
            float lateralSpeed = glm::length(glm::vec2(instantVelocity.x, instantVelocity.z));
            
            // Create ripples based on lateral movement speed
            if (lateralSpeed > 0.2f) { // Lower threshold for more responsive waves
                // Create ripple at sphere position
                glm::vec3 ripplePos(sphere->getPosition().x, waterHeight, sphere->getPosition().z);
                
                // Create continuous small ripples when dragging
                float rippleMagnitude = std::min(lateralSpeed * 0.05f, 0.2f);
                
                // Add ripples less frequently to avoid too many overlapping waves
                static float rippleTimer = 0.0f;
                rippleTimer += deltaTime;
                if (rippleTimer > 0.1f) { // Create ripple every 100ms
                    // Create directional wave based on movement direction
                    glm::vec2 moveDirection = glm::normalize(glm::vec2(instantVelocity.x, instantVelocity.z));
                    simulationManager->addDirectionalRipple(ripplePos, moveDirection, rippleMagnitude);
                    rippleTimer = 0.0f;
                }
                
                // Add water flow impulse based on sphere movement
                glm::vec2 lateralVelocity(instantVelocity.x, instantVelocity.z);
                simulationManager->addWaterFlowImpulse(sphere->getPosition(), lateralVelocity * 0.5f, sphereRadius * 2.5f);
                
                // Create splash effect for high speeds
                if (lateralSpeed > 5.0f) { // Higher threshold for splashes
                    static float splashTimer = 0.0f;
                    splashTimer += deltaTime;
                    if (splashTimer > 0.3f) { // Less frequent splashes
                        simulationManager->createSplash(ripplePos, lateralSpeed * 0.15f);
                        splashTimer = 0.0f;
                    }
                }
            }
        }
        
        lastMouseWorldPos = worldPos;
    } else if (isRightMousePressed) { // Only rotate camera when right mouse button is pressed
        // Normal camera rotation
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // Check if ImGui is using the mouse
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
    
    // Get the current cursor position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Cast a ray into the scene to identify what was clicked
            // Use an initial depth point to check if we're over water
            float rayDepth = 20.0f; // Initial depth to test intersection
            glm::vec3 worldPos = screenToWorld(static_cast<float>(xpos), static_cast<float>(ypos), rayDepth);
            
            // Calculate ray from camera to worldPos
            glm::vec3 rayDir = glm::normalize(worldPos - camera.Position);
            
            // Define water plane (at current water height, with normal pointing up)
            float waterHeight = simulationManager->getWaterHeight();
            glm::vec3 waterPoint(0.0f, waterHeight, 0.0f);
            glm::vec3 waterNormal(0.0f, 1.0f, 0.0f);
            
            // Calculate if and where ray intersects water plane
            float denom = glm::dot(waterNormal, rayDir);
            if (std::abs(denom) > 0.0001f) { // Avoid division by zero
                // Calculate the distance along ray to hit water plane
                float t = glm::dot(waterPoint - camera.Position, waterNormal) / denom;
                
                if (t > 0.0f) { // Check if water is in front of camera
                    // Calculate exact intersection point
                    glm::vec3 waterIntersect = camera.Position + rayDir * t;
                    
                    // Check if intersection is within the container bounds
                    float containerHalfWidth = container->getWidth() / 2.0f;
                    float containerHalfDepth = container->getDepth() / 2.0f;
                    
                    if (std::abs(waterIntersect.x) < containerHalfWidth && 
                        std::abs(waterIntersect.z) < containerHalfDepth) {
                        
                        // Check if we're clicking the water or trying to grab the sphere
                        // Sphere selection has priority if close enough to click point
                        glm::vec3 spherePos = sphere->getPosition();
                        float distToSphere = glm::length(glm::vec2(waterIntersect.x - spherePos.x, 
                                                                  waterIntersect.z - spherePos.z));
                        
                        if (distToSphere < sphere->getRadius() * 1.5f) {
                            // Close enough to grab the sphere
                            isDraggingSphere = true;
                            sphere->setDragged(true);
                            
                            // Store initial world position for dragging
                            float depth = glm::length(camera.Position - sphere->getPosition());
                            lastMouseWorldPos = screenToWorld(static_cast<float>(xpos), static_cast<float>(ypos), depth);
                            
                            // Reset velocity tracking
                            previousSpherePos = sphere->getPosition();
                            dragVelocityTrackTime = 0.0f;
                        } else {
                            // Start creating a ripple at the intersection point
                            isCreatingRipple = true;
                            rippleHoldTime = 0.0f;
                            ripplePosition = waterIntersect;
                            
                            // Create an initial small interaction
                            simulationManager->addRipple(waterIntersect, 0.1f);
                        }
                    }
                }
            }
            
            // If we didn't click water or sphere above, check for direct sphere click
            if (!isDraggingSphere && !isCreatingRipple) {
                // Try raycast against sphere for direct selection
                // Simple implementation: if cursor is "close enough" to sphere on screen, select it
                float depth = glm::length(camera.Position - sphere->getPosition());
                glm::vec3 sphereScreenPos = sphere->getPosition();
                worldPos = screenToWorld(static_cast<float>(xpos), static_cast<float>(ypos), depth);
                
                float distToSphere = glm::length(worldPos - sphere->getPosition());
                if (distToSphere < sphere->getRadius() * 1.5f) {
                    isDraggingSphere = true;
                    sphere->setDragged(true);
                    lastMouseWorldPos = worldPos;
                    
                    // Reset velocity tracking
                    previousSpherePos = sphere->getPosition();
                    dragVelocityTrackTime = 0.0f;
                }
            }
        } else if (action == GLFW_RELEASE) {
            // If we were creating a ripple, create the final interaction with the accumulated magnitude
            if (isCreatingRipple) {
                float rippleMagnitude = 0.1f + rippleHoldTime * RIPPLE_CHARGE_RATE;
                simulationManager->addRipple(ripplePosition, rippleMagnitude);
                isCreatingRipple = false;
            }
            
            if (isDraggingSphere) {
                // Calculate final velocity based on recent movement
                glm::vec3 currentPos = sphere->getPosition();
                glm::vec3 dragVelocity = (currentPos - previousSpherePos) / std::max(dragVelocityTrackTime, 0.016f);
                
                // Apply some dampening to make it more realistic
                dragVelocity *= 0.5f;
                
                // Set the sphere's velocity
                sphere->setVelocity(dragVelocity);
            }
            
            isDraggingSphere = false;
            sphere->setDragged(false);
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        // Track right mouse button state
        isRightMousePressed = (action == GLFW_PRESS);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

float calculateFPS(float deltaTime) {
    static float timeElapsed = 0.0f;
    static int frameCount = 0;
    static float fps = 0.0f;
    
    frameCount++;
    timeElapsed += deltaTime;
    
    if (timeElapsed >= 1.0f) {
        fps = frameCount / timeElapsed;
        frameCount = 0;
        timeElapsed = 0.0f;
    }
    
    return fps;
}

void renderUI(float deltaTime) {
    // Create a window
    ImGui::Begin("Water Simulation Controls");
    
    // FPS counter
    ImGui::Text("FPS: %.1f", calculateFPS(deltaTime));
    
    // Camera position
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", camera.Position.x, camera.Position.y, camera.Position.z);
    
    // Camera controls section
    ImGui::Separator();
    ImGui::Text("Camera Controls");
    
    // Camera mode toggle
    static const char* cameraModes[] = { "Free Camera", "Orbit Camera" };
    static int currentCameraMode = 0;
    if (ImGui::Combo("Camera Mode", &currentCameraMode, cameraModes, 2)) {
        camera.SetMode(currentCameraMode == 0 ? FREE_CAMERA : ORBIT_CAMERA);
        if (currentCameraMode == 1) {
            // Set orbit center to pool center
            camera.SetOrbitCenter(glm::vec3(0.0f, 0.0f, 0.0f));
        }
    }
    
    // Camera zoom/distance slider
    if (camera.Mode == ORBIT_CAMERA) {
        float orbitDistance = camera.OrbitDistance;
        if (ImGui::SliderFloat("Orbit Distance", &orbitDistance, 1.0f, 50.0f)) {
            camera.SetZoom(orbitDistance);
        }
    } else {
        float zoom = camera.Zoom;
        if (ImGui::SliderFloat("Field of View", &zoom, 1.0f, 120.0f)) {
            camera.SetZoom(zoom);
        }
    }
    
    // Camera movement speed slider
    float cameraSpeed = camera.MovementSpeed;
    if (ImGui::SliderFloat("Movement Speed", &cameraSpeed, 1.0f, 10.0f)) {
        camera.MovementSpeed = cameraSpeed;
    }
    
    // Add isometric camera angle button
    if (ImGui::Button("Set Isometric View")) {
        // Set camera to isometric angle (similar to the JavaScript version)
        camera.Position = glm::vec3(0.0f, 8.0f, 12.0f);
        camera.Yaw = -90.0f;  // Looking toward -Z
        camera.Pitch = -35.0f; // Looking down at an angle
        camera.updateCameraVectors(); // Need to make this public or add a wrapper
    }
    
    // Add top-down camera angle button
    ImGui::SameLine();
    if (ImGui::Button("Set Top-Down View")) {
        // Set camera to top-down angle
        camera.Position = glm::vec3(0.0f, 15.0f, 0.0f);
        camera.Yaw = -90.0f;  // Looking toward -Z
        camera.Pitch = -90.0f; // Looking straight down
        camera.updateCameraVectors();
    }
    
    // Sphere settings
    ImGui::Separator();
    ImGui::Text("Sphere Settings");
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", 
                sphere->getPosition().x, sphere->getPosition().y, sphere->getPosition().z);
    
    // Sphere appearance
    ImGui::Checkbox("Mirror Reflections", &enableSphereReflections);
    if (enableSphereReflections) {
        ImGui::SliderFloat("Reflectivity", &sphereReflectivity, 0.0f, 1.0f);
    }
    
    // Gravity toggle
    ImGui::Checkbox("Enable Gravity", &useGravity);
    
    // Gravity strength
    ImGui::SliderFloat("Gravity", &gravity, 1.0f, 20.0f);
    
    // Water properties
    ImGui::Separator();
    ImGui::Text("Water Properties");
    
    // Add splash controls section
    ImGui::Separator();
    ImGui::Text("Splash Controls");
    
    // Button to create an impact splash at the sphere position - simulating JavaScript version's click functionality
    if (ImGui::Button("Create Splash at Sphere")) {
        // Generate an impact splash at the sphere position with a random strength
        float randomStrength = 1.0f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f;
        simulationManager->createSplash(sphere->getPosition(), randomStrength);
    }
    
    // Water color (only for regular water)
    if (simulationManager->isRegularWaterActive()) {
        WaterSurface* waterSurface = simulationManager->getWaterSurface();
        if (waterSurface) {
            glm::vec3 waterColor = waterSurface->getColor();
            float color[3] = { waterColor.r, waterColor.g, waterColor.b };
            if (ImGui::ColorEdit3("Water Color", color)) {
                waterSurface->setColor(glm::vec3(color[0], color[1], color[2]));
            }
            
            // Water transparency
            float transparency = waterSurface->getTransparency();
            if (ImGui::SliderFloat("Transparency", &transparency, 0.0f, 1.0f)) {
                waterSurface->setTransparency(transparency);
            }
        }
    }
    
    // Water simulation mode
    ImGui::Separator();
    ImGui::Text("Water Simulation");
    
    // Water height
    float waterHeight = simulationManager->getWaterHeight();
    if (ImGui::SliderFloat("Water Level", &waterHeight, -3.0f, 3.0f)) {
        simulationManager->setWaterHeight(waterHeight);
    }
    
    // Wave parameters
    ImGui::Separator();
    ImGui::Text("Surface Wave Parameters");
    
    if (simulationManager->isRegularWaterActive()) {
        if (ImGui::Button("Add Random Wave")) {
            WaterSurface* waterSurface = simulationManager->getWaterSurface();
            if (waterSurface) {
                WaterSurface::WaveParam newWave;
                newWave.direction = glm::normalize(glm::vec2(std::rand() / (float)RAND_MAX * 2.0f - 1.0f, 
                                                             std::rand() / (float)RAND_MAX * 2.0f - 1.0f));
                newWave.amplitude = 0.1f + std::rand() / (float)RAND_MAX * 0.2f;
                newWave.wavelength = 2.0f + std::rand() / (float)RAND_MAX * 3.0f;
                newWave.speed = 0.5f + std::rand() / (float)RAND_MAX;
                newWave.steepness = 0.3f + std::rand() / (float)RAND_MAX * 0.3f;
                
                waterSurface->addWave(newWave);
            }
        }
        
        if (ImGui::Button("Reset Waves")) {
            WaterSurface* waterSurface = simulationManager->getWaterSurface();
            if (waterSurface) {
                waterSurface->clearWaves();
                
                // Add default wave
                WaterSurface::WaveParam defaultWave;
                defaultWave.direction = glm::normalize(glm::vec2(1.0f, 1.0f));
                defaultWave.amplitude = 0.1f;
                defaultWave.wavelength = 4.0f;
                defaultWave.speed = 1.0f;
                defaultWave.steepness = 0.5f;
                waterSurface->addWave(defaultWave);
            }
        }
    } else {
        ImGui::TextDisabled("Wave controls are only available for Regular Water simulation");
    }
    
    // Simulation Type Controls
    ImGui::Separator();
    ImGui::Text("Simulation Type");
    
    // Show current simulation type
    const char* currentTypeName = "None";
    switch (simulationManager->getCurrentType()) {
        case WaterSim::SimulationType::REGULAR_WATER:
            currentTypeName = "Regular Water Surface";
            break;
        case WaterSim::SimulationType::SPH_COMPUTE:
            currentTypeName = "SPH Fluid Simulation";
            break;
        case WaterSim::SimulationType::NONE:
        default:
            currentTypeName = "None Selected";
            break;
    }
    ImGui::Text("Current: %s", currentTypeName);
    ImGui::Text("Press ESC to open the main menu to switch simulations");
    
    // SPH-specific controls
    if (simulationManager->isSPHComputeActive()) {
        ImGui::Separator();
        ImGui::Text("SPH Controls");
        
        // SPH Compute specific controls
        WaterSim::SPHComputeSystem* sphComputeSystem = simulationManager->getSPHComputeSystem();
        if (sphComputeSystem) {
                ImGui::Text("Active Particles: %d", sphComputeSystem->getParticleCount());
                
                // Physics parameters
                if (ImGui::CollapsingHeader("Physics Parameters")) {
                    glm::vec3 gravity = sphComputeSystem->getGravity();
                    bool gravityChanged = false;
                    
                    ImGui::Text("Gravity Direction:");
                    if (ImGui::SliderFloat("Gravity X (Left/Right)", &gravity.x, -20.0f, 20.0f)) {
                        gravityChanged = true;
                    }
                    if (ImGui::SliderFloat("Gravity Y (Up/Down)", &gravity.y, -20.0f, 20.0f)) {
                        gravityChanged = true;
                    }
                    if (ImGui::SliderFloat("Gravity Z (Forward/Back)", &gravity.z, -20.0f, 20.0f)) {
                        gravityChanged = true;
                    }
                    
                    if (gravityChanged) {
                        sphComputeSystem->setGravity(gravity);
                    }
                    
                    // Preset gravity directions
                    if (ImGui::Button("Down")) {
                        sphComputeSystem->setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Up")) {
                        sphComputeSystem->setGravity(glm::vec3(0.0f, 9.81f, 0.0f));
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Left")) {
                        sphComputeSystem->setGravity(glm::vec3(-9.81f, 0.0f, 0.0f));
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Right")) {
                        sphComputeSystem->setGravity(glm::vec3(9.81f, 0.0f, 0.0f));
                    }
                    
                    if (ImGui::Button("Zero Gravity")) {
                        sphComputeSystem->setGravity(glm::vec3(0.0f, 0.0f, 0.0f));
                    }
                }
                
                // Rendering options
                if (ImGui::CollapsingHeader("Rendering Options")) {
                    static int colorMode = 0;
                    const char* colorModes[] = { "Normal", "Velocity", "Density", "Pressure" };
                    if (ImGui::Combo("Color Mode", &colorMode, colorModes, 4)) {
                        sphComputeSystem->setColorMode(static_cast<WaterSim::SPHComputeSystem::ColorMode>(colorMode));
                    }
                    
                    static bool useFilteredViscosity = true;
                    if (ImGui::Checkbox("Filtered Viscosity", &useFilteredViscosity)) {
                        sphComputeSystem->setUseFilteredViscosity(useFilteredViscosity);
                    }
                    
                    static int curvatureFlowIterations = 50;
                    if (ImGui::SliderInt("Curvature Flow Iterations", &curvatureFlowIterations, 0, 100)) {
                        sphComputeSystem->setCurvatureFlowIterations(curvatureFlowIterations);
                    }
                }
                
                // Gravity controls
                if (ImGui::CollapsingHeader("Gravity Controls")) {
                    glm::vec3 gravity = sphComputeSystem->getGravity();
                    
                    if (ImGui::SliderFloat("Gravity X (Left/Right)", &gravity.x, -20.0f, 20.0f)) {
                        sphComputeSystem->setGravity(gravity);
                    }
                    if (ImGui::SliderFloat("Gravity Y (Up/Down)", &gravity.y, -20.0f, 20.0f)) {
                        sphComputeSystem->setGravity(gravity);
                    }
                    if (ImGui::SliderFloat("Gravity Z (Forward/Back)", &gravity.z, -20.0f, 20.0f)) {
                        sphComputeSystem->setGravity(gravity);
                    }
                    
                    // Preset gravity directions
                    if (ImGui::Button("Gravity Down")) {
                        sphComputeSystem->setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Gravity Up")) {
                        sphComputeSystem->setGravity(glm::vec3(0.0f, 9.81f, 0.0f));
                    }
                    
                    if (ImGui::Button("Gravity Left")) {
                        sphComputeSystem->setGravity(glm::vec3(-9.81f, 0.0f, 0.0f));
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Gravity Right")) {
                        sphComputeSystem->setGravity(glm::vec3(9.81f, 0.0f, 0.0f));
                    }
                    
                    if (ImGui::Button("Zero Gravity")) {
                        sphComputeSystem->setGravity(glm::vec3(0.0f, 0.0f, 0.0f));
                    }
                }
                
                // Container rendering toggle
                bool renderContainer = sphComputeSystem->getRenderContainer();
                if (ImGui::Checkbox("Render Container", &renderContainer)) {
                    sphComputeSystem->setRenderContainer(renderContainer);
                }
                
                // Reset button
                if (ImGui::Button("Reset Simulation")) {
                    sphComputeSystem->reset();
                }
                
                // Particle emission controls
                if (ImGui::CollapsingHeader("Particle Emission")) {
                    static bool continuousStream = false;
                    static float streamRate = 10.0f;
                    ImGui::Checkbox("Continuous Particle Stream", &continuousStream);
                    if (continuousStream) {
                        ImGui::SliderFloat("Stream Rate", &streamRate, 1.0f, 50.0f, "%.1f particles/sec");
                        simulationManager->addFluidStream(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), streamRate * deltaTime);
                    }
                }
            }
        }
    
    // Advanced rendering controls
    ImGui::Separator();
    ImGui::Text("Advanced Rendering");
    
    // Post-processing effects
    static bool bloomEnabled = true;
    static bool dofEnabled = false;
    static bool volumetricEnabled = true;
    static float bloomThreshold = 1.0f;
    static float bloomIntensity = 0.5f;
    static float focusDistance = 10.0f;
    static float focusRange = 5.0f;
    
    if (ImGui::Checkbox("Enable Bloom", &bloomEnabled)) {
        if (postProcessManager) postProcessManager->setBloomEnabled(bloomEnabled);
    }
    
    if (ImGui::Checkbox("Enable Depth of Field", &dofEnabled)) {
        if (postProcessManager) postProcessManager->setDOFEnabled(dofEnabled);
    }
    
    if (ImGui::Checkbox("Enable Volumetric Lighting", &volumetricEnabled)) {
        if (postProcessManager) postProcessManager->setVolumetricLightingEnabled(volumetricEnabled);
    }
    
    if (bloomEnabled) {
        if (ImGui::SliderFloat("Bloom Threshold", &bloomThreshold, 0.0f, 3.0f)) {
            if (postProcessManager) postProcessManager->setBloomParams(bloomThreshold, bloomIntensity);
        }
        if (ImGui::SliderFloat("Bloom Intensity", &bloomIntensity, 0.0f, 2.0f)) {
            if (postProcessManager) postProcessManager->setBloomParams(bloomThreshold, bloomIntensity);
        }
    }
    
    if (dofEnabled) {
        if (ImGui::SliderFloat("Focus Distance", &focusDistance, 1.0f, 50.0f)) {
            if (postProcessManager) postProcessManager->setDOFParams(focusDistance, focusRange);
        }
        if (ImGui::SliderFloat("Focus Range", &focusRange, 1.0f, 20.0f)) {
            if (postProcessManager) postProcessManager->setDOFParams(focusDistance, focusRange);
        }
    }
    
    // Ray Tracing Controls
    ImGui::Separator();
    ImGui::Text("Real-Time Ray Tracing");
    
    // Use global ray tracing variables
    const char* qualityItems[] = { "OFF", "LOW", "MEDIUM", "HIGH", "ULTRA" };
    
    if (ImGui::Checkbox("Enable Ray Tracing", &rayTracingEnabled)) {
        if (rayTracingManager) {
            if (rayTracingEnabled) {
                // If enabling ray tracing and quality is OFF, set to LOW by default
                if (rayTracingQuality == 0) {
                    rayTracingQuality = 1; // LOW quality
                }
                rayTracingManager->setQuality(static_cast<WaterSim::RayTracingQuality>(rayTracingQuality));
                std::cout << "Ray tracing ENABLED with quality: " << rayTracingQuality << std::endl;
            } else {
                rayTracingManager->setQuality(WaterSim::RayTracingQuality::OFF);
                std::cout << "Ray tracing DISABLED" << std::endl;
            }
        }
    }
    
    if (rayTracingEnabled && rayTracingManager) {
        if (ImGui::Combo("Quality", &rayTracingQuality, qualityItems, IM_ARRAYSIZE(qualityItems))) {
            rayTracingManager->setQuality(static_cast<WaterSim::RayTracingQuality>(rayTracingQuality));
            std::cout << "Ray tracing quality changed to: " << rayTracingQuality << " (" << qualityItems[rayTracingQuality] << ")" << std::endl;
        }
        
        // Ray tracing features
        static bool reflections = true, refractions = true, caustics = true;
        static float reflectionStrength = 1.0f, refractionStrength = 1.0f, causticStrength = 1.0f;
        
        if (ImGui::TreeNode("Ray Tracing Features")) {
            if (ImGui::Checkbox("Reflections", &reflections) ||
                ImGui::Checkbox("Refractions", &refractions) ||
                ImGui::Checkbox("Caustics", &caustics)) {
                WaterSim::RayTracingFeatures features;
                features.reflections = reflections;
                features.refractions = refractions;
                features.caustics = caustics;
                rayTracingManager->setFeatures(features);
            }
            
            ImGui::SliderFloat("Reflection Strength", &reflectionStrength, 0.0f, 2.0f);
            ImGui::SliderFloat("Refraction Strength", &refractionStrength, 0.0f, 2.0f);
            ImGui::SliderFloat("Caustic Strength", &causticStrength, 0.0f, 2.0f);
            
            // Performance info
            ImGui::Separator();
            ImGui::Text("Performance: %.2f ms/frame", rayTracingManager->getLastFrameTime());
            ImGui::Text("Rays/sec: %d", rayTracingManager->getRaysPerSecond());
            
            ImGui::TreePop();
        }
    }

    // Wave controls
    if (simulationManager->isRegularWaterActive()) {
        if (ImGui::Button("Remove All Waves")) {
            WaterSurface* waterSurface = simulationManager->getWaterSurface();
            if (waterSurface) {
                waterSurface->clearWaves();
            }
        }
        
        // Micro-wave toggle
        ImGui::Checkbox("Enable Micro Detail", &enableMicroWaves);
        
        // List all current waves
        WaterSurface* waterSurface = simulationManager->getWaterSurface();
        if (waterSurface) {
            auto& waves = waterSurface->getWaves();
            for (size_t i = 0; i < waves.size(); i++) {
                ImGui::PushID(static_cast<int>(i));
                ImGui::Separator();
                
                ImGui::Text("Wave %zu", i + 1);
                
                float direction[2] = { waves[i].direction.x, waves[i].direction.y };
                if (ImGui::SliderFloat2("Direction", direction, -1.0f, 1.0f)) {
                    if (glm::length(glm::vec2(direction[0], direction[1])) > 0.0f) {
                        waves[i].direction = glm::normalize(glm::vec2(direction[0], direction[1]));
                    }
                }
                
                ImGui::SliderFloat("Amplitude", &waves[i].amplitude, 0.0f, 0.5f);
                ImGui::SliderFloat("Wavelength", &waves[i].wavelength, 1.0f, 10.0f);
                ImGui::SliderFloat("Speed", &waves[i].speed, 0.1f, 3.0f);
                ImGui::SliderFloat("Steepness", &waves[i].steepness, 0.0f, 1.0f);
                
                ImGui::PopID();
            }
        }
    }
    
    ImGui::End();
    
    // Draw ripple indicator if creating ripple
    if (isCreatingRipple) {
        // Set position at bottom center of screen
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH / 2 - 100, SCR_HEIGHT - 40));
        ImGui::SetNextWindowSize(ImVec2(200, 30));
        ImGui::Begin("RippleStrength", nullptr, 
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoInputs | 
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing);
        
        // Calculate ripple strength as percentage
        float strength = (rippleHoldTime * RIPPLE_CHARGE_RATE) / MAX_RIPPLE_MAGNITUDE * 100.0f;
        ImGui::ProgressBar(strength / 100.0f, ImVec2(-1, 0), 
                          ("Ripple Strength: " + std::to_string((int)strength) + "%").c_str());
        
        ImGui::End();
    }
    
}

// Create a more detailed environment map for better reflections
unsigned int loadSkybox(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    
    // Create a higher resolution environment for better reflections
    const int texSize = 256;
    std::vector<unsigned char> data(texSize * texSize * 4);
    
    // Generate different patterns for each face to create a more interesting reflection
    for (unsigned int face = 0; face < 6; face++) {
        for (int y = 0; y < texSize; y++) {
            for (int x = 0; x < texSize; x++) {
                int idx = (y * texSize + x) * 4;
                float u = x / (float)(texSize - 1);
                float v = y / (float)(texSize - 1);
                
                // Different pattern for each face
                if (face == 2) { // Top face (GL_TEXTURE_CUBE_MAP_POSITIVE_Y)
                    // Bright sky with more detailed clouds
                    float cloud1 = sin(u * 10.0f) * sin(v * 10.0f) * 0.15f;
                    float cloud2 = sin(u * 25.0f + 1.5f) * sin(v * 25.0f + 1.5f) * 0.05f;
                    float cloudValue = 0.85f + cloud1 + cloud2;
                    data[idx + 0] = (unsigned char)(180 * cloudValue);
                    data[idx + 1] = (unsigned char)(210 * cloudValue);
                    data[idx + 2] = (unsigned char)(255 * cloudValue);
                } else if (face == 3) { // Bottom face (GL_TEXTURE_CUBE_MAP_NEGATIVE_Y)
                    // Dark floor for contrast
                    data[idx + 0] = 40;
                    data[idx + 1] = 40;
                    data[idx + 2] = 50;
                } else {
                    // Side faces - gradient with some variation
                    float height = 1.0f - v;
                    float variation = sin(u * 20.0f) * 0.05f;
                    
                    // Sky gradient with building-like structures
                    if (v > 0.6f) {
                        // Sky
                        data[idx + 0] = (unsigned char)(135 + 120 * height + variation * 20);
                        data[idx + 1] = (unsigned char)(206 + 49 * height + variation * 20);
                        data[idx + 2] = (unsigned char)(235 + 20 * height);
                    } else {
                        // Building silhouettes for more interesting reflections
                        float building = (sin(u * 15.0f) > 0.5f && v < 0.4f) ? 0.3f : 1.0f;
                        data[idx + 0] = (unsigned char)(100 * building);
                        data[idx + 1] = (unsigned char)(100 * building);
                        data[idx + 2] = (unsigned char)(120 * building);
                    }
                }
                data[idx + 3] = 255;
            }
        }
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA, texSize, texSize, 
                     0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    return textureID;
}

// Add a new function to create dummy reflection/refraction textures
unsigned int createDummyTexture() {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Create a simple blue-white gradient texture
    const int texSize = 256;
    unsigned char data[texSize * texSize * 4];
    
    for (int y = 0; y < texSize; y++) {
        for (int x = 0; x < texSize; x++) {
            float t = y / (float)(texSize - 1); // Gradient from bottom to top
            
            // Blue-white gradient for water
            data[(y * texSize + x) * 4 + 0] = (unsigned char)(155 + 100 * t); // R
            data[(y * texSize + x) * 4 + 1] = (unsigned char)(196 + 59 * t);  // G
            data[(y * texSize + x) * 4 + 2] = (unsigned char)(225 + 30 * t);  // B
            data[(y * texSize + x) * 4 + 3] = 255;                            // A
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    enableAnisotropicFiltering();
    
    return textureID;
}

// Replace the createCausticTexture function with this improved version
unsigned int createCausticTexture(int size) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Create a high-contrast caustic texture with strong light patterns
    std::vector<unsigned char> data(size * size * 4);
    
    // Use better randomization for the noise functions
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Generate multiple layers of noise to create a more natural caustic
    std::vector<std::vector<float>> noise1(size, std::vector<float>(size, 0.0f));
    std::vector<std::vector<float>> noise2(size, std::vector<float>(size, 0.0f));
    std::vector<std::vector<float>> noise3(size, std::vector<float>(size, 0.0f));
    
    // First layer - large scale features
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float nx = (float)x / size * 4.0f;
            float ny = (float)y / size * 4.0f;
            
            float val = 0.5f + 0.5f * sin(nx * 3.14159f) * sin(ny * 3.14159f);
            noise1[y][x] = val;
        }
    }
    
    // Second layer - medium scale features
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float nx = (float)x / size * 8.0f;
            float ny = (float)y / size * 8.0f;
            
            float val = 0.5f + 0.5f * sin(nx * 3.14159f + 0.5f) * sin(ny * 3.14159f + 1.5f);
            noise2[y][x] = val;
        }
    }
    
    // Third layer - small scale features
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float nx = (float)x / size * 16.0f;
            float ny = (float)y / size * 16.0f;
            
            float val = 0.5f + 0.5f * sin(nx * 3.14159f + 1.0f) * sin(ny * 3.14159f + 2.0f);
            noise3[y][x] = val;
        }
    }
    
    // Combine the noise layers and apply contrast enhancement
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Combine noise layers with weights
            float combinedNoise = noise1[y][x] * 0.5f + noise2[y][x] * 0.3f + noise3[y][x] * 0.2f;
            
            // Apply distortion to create more realistic caustic patterns
            float distX = 0.05f * sin(noise2[y][x] * 10.0f);
            float distY = 0.05f * sin(noise1[y][x] * 10.0f);
            
            int sampleX = std::min(size - 1, std::max(0, x + (int)(distX * size)));
            int sampleY = std::min(size - 1, std::max(0, y + (int)(distY * size)));
            
            float distortedNoise = noise1[sampleY][sampleX] * 0.6f + noise3[sampleY][sampleX] * 0.4f;
            
            // Create sharpened caustic-like effect
            float caustic = std::pow(distortedNoise, 4.0f); // Sharpen the effect
            
            // Add random sharp caustic edges
            float sharpEdge = 0.0f;
            if (caustic > 0.5f && caustic < 0.55f)
                sharpEdge = 0.5f;
            
            caustic = std::min(1.0f, caustic + sharpEdge);
            
            // Store in texture with bluish tint for underwater effect
            int idx = (y * size + x) * 4;
            data[idx] = (unsigned char)(std::min(caustic * 180.0f, 255.0f)); // R - less red
            data[idx + 1] = (unsigned char)(std::min(caustic * 230.0f, 255.0f)); // G - more green
            data[idx + 2] = (unsigned char)(std::min(caustic * 255.0f, 255.0f)); // B - most blue
            data[idx + 3] = 255; // Alpha
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    enableAnisotropicFiltering();
    
    return textureID;
}

// TODO: Add this function after the createCausticTexture function
unsigned int createTileTexture(int size) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Create procedural tile texture (white tiles with dark grout)
    std::vector<unsigned char> data(size * size * 4);
    
    int tileSize = size / 16;
    int groutWidth = tileSize / 8;
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int idx = (y * size + x) * 4;
            
            // Calculate tile grid position
            int gridX = x % tileSize;
            int gridY = y % tileSize;
            
            // Determine if we're in a grout line
            bool inGrout = (gridX < groutWidth || gridX >= tileSize - groutWidth || 
                            gridY < groutWidth || gridY >= tileSize - groutWidth);
            
            // Slightly vary the tile color and grout color
            float noise = (float)((x * 17 + y * 29) % 10) / 100.0f;
            
            if (inGrout) {
                // Dark grout color
                data[idx] = 80 + (int)(noise * 20);
                data[idx + 1] = 80 + (int)(noise * 20);
                data[idx + 2] = 80 + (int)(noise * 20);
            } else {
                // White tile with slight variation
                data[idx] = 240 + (int)(noise * 15);
                data[idx + 1] = 240 + (int)(noise * 15);
                data[idx + 2] = 240 + (int)(noise * 15);
            }
            
            // Add a subtle highlight to the tiles
            if (!inGrout) {
                // Calculate distance from tile center
                float tileU = (float)(gridX - tileSize/2) / (tileSize/2);
                float tileV = (float)(gridY - tileSize/2) / (tileSize/2);
                float dist = sqrt(tileU*tileU + tileV*tileV);
                
                // Add highlight based on distance from center
                float highlight = std::max(0.0f, 1.0f - dist*1.2f);
                data[idx] = std::min(255, data[idx] + (int)(highlight * 15));
                data[idx + 1] = std::min(255, data[idx + 1] + (int)(highlight * 15));
                data[idx + 2] = std::min(255, data[idx + 2] + (int)(highlight * 15));
            }
            
            data[idx + 3] = 255; // Alpha
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    enableAnisotropicFiltering();
    
    return textureID;
}

// Create a realistic steel/metal texture with reflective properties
unsigned int createSteelTexture(int size) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Create a procedural steel texture with brushed metal appearance
    std::vector<unsigned char> data(size * size * 4);
    
    // Generate brushed metal pattern
    std::mt19937 rng(12345); // Fixed seed for consistent pattern
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Create base noise for surface irregularities
    std::vector<std::vector<float>> baseNoise(size, std::vector<float>(size));
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            baseNoise[y][x] = dist(rng);
        }
    }
    
    // Apply horizontal brushing effect
    for (int y = 0; y < size; y++) {
        // Create horizontal streaks
        float streakIntensity = 0.5f + 0.5f * sin(y * 0.1f);
        
        for (int x = 0; x < size; x++) {
            // Base metal color (slightly bluish-gray)
            float r = 0.7f;
            float g = 0.72f;
            float b = 0.75f;
            
            // Add brushed metal texture
            float brushNoise = 0.0f;
            for (int i = -2; i <= 2; i++) {
                int sampleX = std::max(0, std::min(size - 1, x + i));
                brushNoise += baseNoise[y][sampleX] * (1.0f - std::abs(i) / 3.0f);
            }
            brushNoise /= 3.0f;
            
            // Add vertical scratches
            float scratch = 0.0f;
            if (dist(rng) < 0.05f) { // 5% chance of scratch
                scratch = 0.1f * (1.0f - std::abs(sin(x * 0.5f)));
            }
            
            // Combine effects
            float metallic = brushNoise * streakIntensity + scratch;
            
            // Apply to color with metallic highlights
            r = std::min(1.0f, r + metallic * 0.3f);
            g = std::min(1.0f, g + metallic * 0.32f);
            b = std::min(1.0f, b + metallic * 0.35f);
            
            // Add specular highlights
            float highlight = std::pow(brushNoise, 3.0f) * 0.2f;
            r = std::min(1.0f, r + highlight);
            g = std::min(1.0f, g + highlight);
            b = std::min(1.0f, b + highlight);
            
            // Convert to byte values
            int idx = (y * size + x) * 4;
            data[idx] = (unsigned char)(r * 255.0f);
            data[idx + 1] = (unsigned char)(g * 255.0f);
            data[idx + 2] = (unsigned char)(b * 255.0f);
            data[idx + 3] = 255; // Full alpha
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Set texture parameters for metallic appearance
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    enableAnisotropicFiltering();
    
    return textureID;
}

// Helper function to enable anisotropic filtering
void enableAnisotropicFiltering() {
    static bool checkedSupport = false;
    static bool hasAnisotropic = false;
    
    if (!checkedSupport) {
        // Check if anisotropic filtering is supported
        hasAnisotropic = glfwExtensionSupported("GL_EXT_texture_filter_anisotropic");
        checkedSupport = true;
        
        if (hasAnisotropic) {
            std::cout << "Anisotropic filtering supported" << std::endl;
        } else {
            std::cout << "Anisotropic filtering not supported" << std::endl;
        }
    }
    
    if (hasAnisotropic) {
        float maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    }
}

// Windows OpenGL debug callback for performance monitoring
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                           GLsizei length, const GLchar* message, const void* userParam) {
    // Ignore non-significant error/warning codes but log performance issues
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    
    // Always log shader compilation/linking errors
    if (source == GL_DEBUG_SOURCE_SHADER_COMPILER || 
        std::string(message).find("shader") != std::string::npos ||
        std::string(message).find("program") != std::string::npos ||
        std::string(message).find("link") != std::string::npos) {
        std::cout << "=== SHADER ERROR ===" << std::endl;
        std::cout << "Message: " << message << std::endl;
        return;
    }
    
    // Focus on performance-related messages for GPU optimization
    bool isPerformanceIssue = (type == GL_DEBUG_TYPE_PERFORMANCE || 
                              severity == GL_DEBUG_SEVERITY_HIGH ||
                              severity == GL_DEBUG_SEVERITY_MEDIUM);
    
    if (isPerformanceIssue) {
        std::cout << "=== GPU PERFORMANCE ALERT ===" << std::endl;
        std::cout << "Message (" << id << "): " << message << std::endl;
        
        switch (source) {
            case GL_DEBUG_SOURCE_API:             std::cout << "Source: OpenGL API"; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
            case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
            case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
        }
        std::cout << " | ";
        
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Critical Error"; break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated (Performance Impact)"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behavior"; break;
            case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
            case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: PERFORMANCE ISSUE"; break;
            case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
            case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
            case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
            case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
        }
        std::cout << " | ";
        
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: HIGH (GPU Performance Critical)"; break;
            case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: MEDIUM (Performance Impact)"; break;
            case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: Low"; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: Notification"; break;
        }
        std::cout << std::endl << "=============================" << std::endl;
    }
}

// Render scene function for reflection/refraction passes and main rendering
void renderScene(const Camera& camera, float waterLevel, bool isReflection, bool isRefraction) {
    // Create transformation matrices
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    
    // Apply reflection transformation if needed
    if (isReflection) {
        // Reflect camera position and view direction across water plane
        glm::mat4 reflectionMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));
        reflectionMatrix = glm::translate(reflectionMatrix, glm::vec3(0.0f, 2.0f * waterLevel, 0.0f));
        view = view * reflectionMatrix;
        
        // Reverse winding order for reflection
        glFrontFace(GL_CW);
    }
    
    // Render sphere
    if (isShaderProgramValid(sphereShader)) {
        glUseProgram(sphereShader);
        
        // Set sphere shader uniforms
        glUniformMatrix4fv(glGetUniformLocation(sphereShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(sphereShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, sphere->getPosition());
        glUniformMatrix4fv(glGetUniformLocation(sphereShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        // Set lighting uniforms
        glUniform3f(glGetUniformLocation(sphereShader, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform3f(glGetUniformLocation(sphereShader, "lightPos"), 5.0f, 10.0f, 5.0f);
        glUniform3f(glGetUniformLocation(sphereShader, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(sphereShader, "ambientStrength"), 0.1f);
        glUniform1f(glGetUniformLocation(sphereShader, "specularStrength"), 0.8f); // Moderate for realistic metal
        glUniform1f(glGetUniformLocation(sphereShader, "shininess"), 128.0f); // High but not excessive for metal
        
        // Enable texture for steel appearance
        glUniform1i(glGetUniformLocation(sphereShader, "useTexture"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, steelTexture);
        glUniform1i(glGetUniformLocation(sphereShader, "sphereTexture"), 0);
        
        // Enable reflections for mirror-like appearance
        glUniform1i(glGetUniformLocation(sphereShader, "enableReflections"), enableSphereReflections ? 1 : 0);
        glUniform1f(glGetUniformLocation(sphereShader, "reflectivity"), sphereReflectivity);
        
        // Bind skybox for environment reflections
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
        glUniform1i(glGetUniformLocation(sphereShader, "skybox"), 1);
        
        // Only render sphere if not doing water passes or if sphere is above/below water appropriately
        bool shouldRenderSphere = true;
        if (isReflection) {
            // Only render sphere in reflection if it's above water
            shouldRenderSphere = sphere->getPosition().y > waterLevel;
        } else if (isRefraction) {
            // Only render sphere in refraction if it's below water
            shouldRenderSphere = sphere->getPosition().y < waterLevel;
        }
        
        if (shouldRenderSphere) {
            sphere->render(sphereShader);
        }
    }
    
    // Don't render glass container here - it's rendered separately in the main loop with proper transparency
    
    // Restore normal winding order if we changed it
    if (isReflection) {
        glFrontFace(GL_CCW);
    }
}

// Update wave simulation using GPU compute shaders
void updateWaveSimulation(float deltaTime, float time) {
    #ifdef GL_COMPUTE_SHADER
    // This will be enabled when GLAD is updated with compute shader support
    
    // Get wave parameters from water surface (if regular water is active)
    if (!simulationManager->isRegularWaterActive()) return;
    
    WaterSurface* waterSurface = simulationManager->getWaterSurface();
    if (!waterSurface) return;
    
    auto& waves = waterSurface->getWaves();
    if (waves.empty()) return;
    
    // TODO: Load compute shader if not already loaded
    static GLuint waveComputeShader = 0;
    if (waveComputeShader == 0) {
        // Load compute shader when available
        // waveComputeShader = InitShader("shaders/wave_compute.cs", GL_COMPUTE_SHADER);
        return;
    }
    
    // Dispatch compute shader
    glUseProgram(waveComputeShader);
    
    // Set uniforms
    glUniform1f(glGetUniformLocation(waveComputeShader, "time"), time);
    glUniform1i(glGetUniformLocation(waveComputeShader, "waveCount"), static_cast<int>(std::min(waves.size(), static_cast<size_t>(16))));
    glUniform2f(glGetUniformLocation(waveComputeShader, "textureSize"), 256.0f, 256.0f);
    glUniform1f(glGetUniformLocation(waveComputeShader, "worldSize"), 10.0f);
    
    // Set wave parameters
    for (int i = 0; i < static_cast<int>(std::min(waves.size(), static_cast<size_t>(16))); i++) {
        // Set wave parameters as uniform arrays
        // This would be implemented when compute shaders are available
    }
    
    // Bind wave height texture as image
    glBindImageTexture(0, waveHeightMap->getTextureID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    
    // Dispatch compute threads
    glDispatchCompute(16, 16, 1); // 256x256 texture with 16x16 local groups
    
    // Memory barrier
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    
    #else
    // Placeholder implementation - update wave height map on CPU
    static std::vector<float> heights(256 * 256, 0.0f);
    
    // Simple CPU-based wave calculation for demonstration
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            float worldX = (x / 256.0f - 0.5f) * 10.0f;
            float worldZ = (y / 256.0f - 0.5f) * 10.0f;
            
            float height = 0.0f;
            auto& waves = waterSurface->getWaves();
            for (const auto& wave : waves) {
                float k = 2.0f * 3.14159f / wave.wavelength;
                float w = sqrt(9.8f * k);
                float phase = k * (wave.direction.x * worldX + wave.direction.y * worldZ) - w * wave.speed * time;
                height += wave.amplitude * sin(phase);
            }
            
            heights[y * 256 + x] = height;
        }
    }
    
    // Update height map texture
    waveHeightMap->updateHeightMap(heights);
    #endif
}
#include "MainMenu.h"
#include <imgui.h>
#include <iostream>

namespace WaterSim {

MainMenu::MainMenu()
    : showMenu_(true)
    , selectedSimulation_(SimulationType::NONE)
    , selectionChanged_(false)
    , selectedIndex_(0)
{
}

MainMenu::~MainMenu() = default;

void MainMenu::render() {
    if (!showMenu_) {
        return;
    }
    
    setupMenuStyle();
    renderBackground();
    renderMainMenuPanel();
}

void MainMenu::setupMenuStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowPadding = ImVec2(20, 20);
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(8, 8);
    
    // Color scheme
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.12f, 0.18f, 0.95f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.25f, 0.35f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.35f, 0.45f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.45f, 0.55f, 1.0f);
}

void MainMenu::renderBackground() {
    // Create a fullscreen background
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGui::Begin("MenuBackground", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Gradient background
    draw_list->AddRectFilledMultiColor(
        ImVec2(0, 0), io.DisplaySize,
        IM_COL32(15, 25, 35, 255),    // Top-left
        IM_COL32(25, 35, 45, 255),    // Top-right
        IM_COL32(35, 45, 55, 255),    // Bottom-right
        IM_COL32(25, 35, 45, 255)     // Bottom-left
    );
    
    ImGui::End();
}

void MainMenu::renderMainMenuPanel() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Center the menu panel
    float windowWidth = 600.0f;
    float windowHeight = 500.0f;
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - windowWidth) * 0.5f, 
                                   (io.DisplaySize.y - windowHeight) * 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
    
    ImGui::Begin("Water Simulation - Main Menu", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    
    // Title
    ImGui::PushFont(nullptr); // Use default font but larger
    ImGui::SetCursorPosX((windowWidth - ImGui::CalcTextSize("Water Simulation").x) * 0.5f);
    ImGui::Text("Water Simulation");
    ImGui::PopFont();
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // Subtitle
    ImGui::SetCursorPosX((windowWidth - ImGui::CalcTextSize("Choose your simulation type:").x) * 0.5f);
    ImGui::Text("Choose your simulation type:");
    ImGui::Spacing();
    ImGui::Spacing();
    
    // Simulation options
    float buttonWidth = windowWidth - 80.0f;
    float buttonHeight = 80.0f;
    
    // Regular Water Simulation
    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    if (ImGui::Button("Regular Water Surface", ImVec2(buttonWidth, buttonHeight))) {
        selectedSimulation_ = SimulationType::REGULAR_WATER;
        selectionChanged_ = true;
        showMenu_ = false;
        std::cout << "Selected: Regular Water Surface Simulation" << std::endl;
    }
    
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Classic water surface with Gerstner waves\nFeatures: Wave animation, ripples, reflections");
    }
    
    ImGui::Spacing();
    
    
    ImGui::Spacing();
    
    // SPH Simulation (GPU-Optimized)
    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    if (ImGui::Button("SPH Fluid Simulation", ImVec2(buttonWidth, buttonHeight))) {
        selectedSimulation_ = SimulationType::SPH_COMPUTE;
        selectionChanged_ = true;
        showMenu_ = false;
        std::cout << "Selected: SPH Fluid Simulation" << std::endl;
    }
    
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("GPU-optimized fluid simulation using compute shaders\nFeatures: Real-time particle physics, flexible gravity, interactive fluid dynamics");
    }
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    // Information section
    renderSimulationInfo();
    
    // Footer
    ImGui::SetCursorPosY(windowHeight - 60.0f);
    ImGui::Separator();
    ImGui::SetCursorPosX((windowWidth - ImGui::CalcTextSize("Press ESC to return to this menu at any time").x) * 0.5f);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press ESC to return to this menu at any time");
    
    ImGui::End();
}

void MainMenu::renderSimulationInfo() {
    ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Simulation Features:");
    
    ImGui::Indent();
    
    // Simulation info
    ImGui::BulletText("Regular Water: Gerstner waves, real-time ripples, surface reflections");
    ImGui::BulletText("SPH Fluid: GPU-accelerated particle simulation with flexible gravity");
    ImGui::BulletText("Both support: Mouse interactions, sphere physics, real-time controls");
    
    ImGui::Unindent();
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.6f, 1.0f), "System Requirements:");
    ImGui::Indent();
    ImGui::BulletText("Windows 10/11 with OpenGL 4.6 support");
    ImGui::BulletText("NVIDIA RTX GPU recommended for CUDA acceleration");
    ImGui::BulletText("DirectX 12 compatible graphics card");
    ImGui::Unindent();
}

const char* MainMenu::getSimulationName(SimulationType type) const {
    switch (type) {
        case SimulationType::REGULAR_WATER: return "Regular Water Surface";
        case SimulationType::SPH_COMPUTE: return "SPH Fluid Simulation";
        case SimulationType::NONE: 
        default: return "None";
    }
}

const char* MainMenu::getSimulationDescription(SimulationType type) const {
    switch (type) {
        case SimulationType::REGULAR_WATER: 
            return "Classic water surface simulation with Gerstner waves and ripple effects";
        case SimulationType::SPH_COMPUTE:
            return "GPU-optimized fluid simulation using compute shaders for real-time particle physics";
        case SimulationType::NONE: 
        default: return "No simulation selected";
    }
}

} // namespace WaterSim
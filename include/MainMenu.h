#pragma once

#include "SimulationManager.h"
#include <memory>

namespace WaterSim {

class MainMenu {
public:
    MainMenu();
    ~MainMenu();
    
    // Menu state
    bool isMenuActive() const { return showMenu_; }
    void setMenuActive(bool active) { showMenu_ = active; }
    
    // Render the menu UI
    void render();
    
    // Get selected simulation type
    SimulationType getSelectedSimulation() const { return selectedSimulation_; }
    bool hasSelectionChanged() const { return selectionChanged_; }
    void clearSelectionChanged() { selectionChanged_ = false; }
    
    // Menu actions
    void reset() { selectedSimulation_ = SimulationType::NONE; selectionChanged_ = false; }

private:
    bool showMenu_;
    SimulationType selectedSimulation_;
    bool selectionChanged_;
    
    // UI state
    int selectedIndex_;
    
    // Menu styling
    void setupMenuStyle();
    void renderBackground();
    void renderMainMenuPanel();
    void renderSimulationInfo();
    
    // Helper methods
    const char* getSimulationName(SimulationType type) const;
    const char* getSimulationDescription(SimulationType type) const;
};

} // namespace WaterSim
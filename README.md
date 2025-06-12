# Water Simulation

A 3D water simulation application built with OpenGL 4.6 and CUDA acceleration for Windows with NVIDIA RTX graphics cards.

## Features
- **Gerstner Wave Physics**: GPU-accelerated ocean wave simulation
- **Dynamic Ripples**: Interactive ripple system with mouse input
- **Post-processing**: Bloom, depth of field, and volumetric lighting effects

## Prerequisites
- **GPU**: NVIDIA RTX series
- **OS**: Windows 10/11 with latest NVIDIA drivers
- **Tools**: Visual Studio 2019+, CUDA Toolkit 11.0+, CMake 3.18+
- **RAM**: 16GB+ recommended

## Building
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
```
Open the `.sln` file in Visual Studio and build.

## Running
```cmd
cd bin
WaterSimulation.exe
```

## Controls
- **W/A/S/D**: Move camera
- **Mouse**: Rotate camera (right button) / Zoom (wheel)
- **Left Click**: Create ripples
- **G**: Toggle gravity
- **R**: Add random wave
- **C**: Reset simulation

## Architecture
- **WaterSurface**: Gerstner wave implementation
- **PostProcessManager**: HDR bloom, depth of field, volumetric lighting
- **Shader System**: Located in `shaders/` directory

## Troubleshooting
- **OpenGL issues**: Update NVIDIA drivers
- **CUDA not found**: Install CUDA Toolkit 11.0+
- **Low performance**: Disable post-processing effects

## Project Report
**COMP 410 - Interactive Water Simulation**  
**Group Members:** Kaan Altaş (79855), Zeynep Öykü Aslan (79731)  
**Date:** June 4, 2025
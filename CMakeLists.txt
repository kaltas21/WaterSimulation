cmake_minimum_required(VERSION 3.18)

set(CMAKE_CUDA_ARCHITECTURES "75;80;86;89;90")

project(WaterSimulation LANGUAGES CXX CUDA)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# CUDA is mandatory for this project
enable_language(CUDA)
find_package(CUDAToolkit 11.0 REQUIRED)

# Windows-specific settings
if(WIN32)
    # Use static runtime to avoid DLL dependencies
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # Windows optimization flags
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /Oi /Ot /GL /fp:fast /arch:AVX2")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3 --use_fast_math")
    
    # Enable parallel compilation for C++ only (not CUDA)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/MP>)
endif()

# Find required packages
find_package(OpenGL REQUIRED)
find_package(OpenCL QUIET)

# Build GLFW from source
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(ext/glfw_source)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/ext/glfw_source/include
    ${CUDAToolkit_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/Camera.cpp
    src/Framebuffer.cpp
    src/GlassContainer.cpp
    src/HeightMapTexture.cpp
    src/InitShader.cpp
    src/PostProcessManager.cpp
    src/ReflectionRenderer.cpp
    src/RayTracingManager.cpp
    src/Skybox.cpp
    src/Sphere.cpp
    src/WaterSurface.cpp
    src/SimulationManager.cpp
    src/MainMenu.cpp
    src/SPHComputeSystem.cpp
    src/glad.c
)

# No platform-specific SPH sources needed anymore
# CUDA is used for general GPU acceleration, not SPH-specific

# CUDA source files (empty for now, kept for future CUDA acceleration)
set(CUDA_SOURCES
)

# ImGui source files
set(IMGUI_SOURCES
    ext/imgui/imgui.cpp
    ext/imgui/imgui_demo.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/backends/imgui_impl_glfw.cpp
    ext/imgui/backends/imgui_impl_opengl3.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES} ${CUDA_SOURCES})

# Add compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_ENABLE_EXPERIMENTAL)

# Set CUDA properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    OpenGL::GL
    glfw
    CUDA::cudart
    CUDA::cuda_driver
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} 
        gdi32
        user32
        kernel32
        shell32
        d3d11
        d3dcompiler
        dxguid
    )
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
)

# Copy shaders to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    ${CMAKE_SOURCE_DIR}/bin/shaders
)

# Copy textures to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/textures
    ${CMAKE_SOURCE_DIR}/bin/textures
)
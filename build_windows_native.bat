@echo off
setlocal enabledelayedexpansion
echo ==========================================
echo Building Water Simulation for Windows
echo ==========================================

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found!
    echo Please run this script from the project root directory.
    exit /b 1
)

REM Try to find Visual Studio using vswhere
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% (
    set VSWHERE="%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)

REM Check if already in VS environment
where cl >nul 2>nul
if %errorlevel% equ 0 (
    echo Visual Studio environment already set up.
    goto :check_cuda
)

REM Find and setup Visual Studio environment
if exist %VSWHERE% (
    echo Looking for Visual Studio installation...
    for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -property installationPath`) do (
        set VS_PATH=%%i
    )
    
    if defined VS_PATH (
        echo Found Visual Studio at: !VS_PATH!
        
        REM Try VS 2022 first
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
            echo Setting up VS 2022 environment...
            call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat"
            goto :check_cuda
        )
        
        REM Try VS 2019
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
            echo Setting up VS 2019 environment...
            call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat"
            goto :check_cuda
        )
    )
)

REM Final check for VS
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Visual Studio C++ compiler not found!
    echo.
    echo Please try one of these solutions:
    echo 1. Install Visual Studio 2019 or newer with "Desktop development with C++" workload
    echo 2. Run this script from a "Developer Command Prompt for VS"
    echo 3. Install Build Tools for Visual Studio from:
    echo    https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
    exit /b 1
)

:check_cuda

REM Check for CUDA
where nvcc >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: CUDA toolkit not found!
    echo Please install CUDA Toolkit 11.0 or newer
    echo Make sure CUDA\bin is in your PATH
    exit /b 1
)

REM Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: CMake not found!
    echo Please install CMake 3.18 or newer
    exit /b 1
)

echo.
echo Found all required tools:
echo - Visual Studio C++ Compiler
echo - CUDA Toolkit
echo - CMake

REM Show CUDA version
echo.
echo CUDA Version:
nvcc --version | findstr "release"

REM Clean previous build
echo.
echo Cleaning previous build...
if exist "build" rmdir /s /q "build"
mkdir build
cd build

REM Copy the Windows native CMakeLists
echo.
echo Using Windows native CMakeLists...
copy /Y ..\CMakeLists_windows_native.txt ..\CMakeLists.txt

REM Configure with CMake
echo.
echo Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_CUDA_ARCHITECTURES="75;80;86;89;90"

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    cd ..
    exit /b 1
)

REM Build
echo.
echo Building project...
cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS%

if %errorlevel% neq 0 (
    echo.
    echo ==========================================
    echo BUILD FAILED!
    echo ==========================================
    cd ..
    exit /b 1
)

echo.
echo ==========================================
echo BUILD SUCCESSFUL!
echo ==========================================
echo Executable: bin\Release\WaterSimulation.exe
echo.
echo To run: bin\Release\WaterSimulation.exe
echo ==========================================

cd ..
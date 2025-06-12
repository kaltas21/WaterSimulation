@echo off
echo ==========================================
echo FAST BUILD - Water Simulation
echo No checks, warnings disabled
echo ==========================================

REM Clean previous build quickly
if exist "build" rmdir /s /q "build" >nul 2>&1
mkdir build >nul 2>&1
cd build

REM Configure with CMake - minimal output, warnings disabled
echo Configuring...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_CUDA_ARCHITECTURES="75;80;86;89;90" ^
    -DCMAKE_CXX_FLAGS="/w" ^
    -DCMAKE_CUDA_FLAGS="--disable-warnings" ^
    -Wno-dev >nul 2>&1

REM Build with all cores, suppress warnings
echo Building...
cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS% >nul 2>&1

if %errorlevel% neq 0 (
    echo BUILD FAILED!
    cd ..
    exit /b 1
)

echo ==========================================
echo FAST BUILD COMPLETE!
echo ==========================================
echo Executable: bin\Release\WaterSimulation.exe
echo ==========================================

cd ..
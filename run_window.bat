@echo off
REM filepath: run.bat
REM Windows batch script to build and run RTSP project

echo ============================================
echo    RTSP Video Streaming - Windows Build
echo ============================================

REM Create build directory
if not exist build mkdir build
cd build

REM Run CMake
echo.
echo [1/3] Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build project
echo.
echo [2/3] Building project...
cmake --build . --config Release -j %NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

REM Copy videos folder
echo.
echo [3/3] Copying video files...
if not exist bin\Release\videos mkdir bin\Release\videos
xcopy /Y /E ..\server\videos\* bin\Release\videos\ 2>nul

echo.
echo ============================================
echo    Build completed successfully!
echo ============================================
echo.
echo To run server:
echo    cd build\bin\Release
echo    server.exe 8554
echo.
echo To run client:
echo    cd build\bin\Release
echo    client.exe 127.0.0.1 8554 movie.Mjpeg 25000
echo.

cd ..
pause
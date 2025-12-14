@echo off
REM ============================================================================
REM  RTSP/RTP Video Streaming - Windows Setup Launcher
REM  This batch file launches the Python setup script
REM ============================================================================

echo.
echo ======================================================================
echo    RTSP/RTP Video Streaming - Windows Setup
echo ======================================================================
echo.

REM Check if Python is installed
where python >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Python is not installed or not in PATH
    echo.
    echo Please install Python 3.7 or later from:
    echo https://www.python.org/downloads/
    echo.
    echo Make sure to check "Add Python to PATH" during installation!
    echo.
    pause
    exit /b 1
)

REM Display Python version
echo [INFO] Python detected:
python --version
echo.

REM Check if setup.py exists
if not exist "setup.py" (
    echo [ERROR] setup.py not found in current directory
    echo.
    echo Please run this script from the project root directory
    echo.
    pause
    exit /b 1
)

REM Run setup.py
echo [INFO] Launching setup script...
echo.
python setup.py

REM Pause to see results
echo.
echo ======================================================================
pause


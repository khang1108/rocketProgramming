#!/usr/bin/env python3
"""
Cross-platform setup script for RTSP/RTP Streaming Project
Automatically detects OS, checks Qt6, installs dependencies, and builds project
"""

import os
import sys
import platform
import subprocess
import shutil
from pathlib import Path

def print_banner():
    """Print setup banner"""
    print("=" * 70)
    print("   RTSP/RTP Video Streaming - Automated Setup Utility")
    print("   Platform Detection | Dependency Check | Build Automation")
    print("=" * 70)
    print()

def detect_platform():
    """Detect operating system"""
    system = platform.system().lower()
    if system == "linux":
        return "linux"
    elif system == "windows":
        return "windows"
    elif system == "darwin":
        return "macos"
    else:
        return "unknown"

def check_qt6_installed():
    """Check if Qt6 is already installed"""
    print("[CHECK] Verifying Qt6 installation...")
    
    # Method 1: Check qmake6
    if shutil.which("qmake6"):
        print("   [OK] Qt6 detected via qmake6")
        return True
    
    # Method 2: Try to import Qt6 in CMake
    try:
        result = subprocess.run(
            ["cmake", "--find-package", "-DNAME=Qt6", "-DCOMPILER_ID=GNU", 
             "-DLANGUAGE=CXX", "-DMODE=EXIST"],
            capture_output=True,
            text=True,
            timeout=5
        )
        if result.returncode == 0:
            print("   [OK] Qt6 detected via CMake")
            return True
    except:
        pass
    
    # Method 3: Check common installation paths
    qt_paths = {
        "windows": [
            r"C:\Qt\6.5.0",
            r"C:\Qt\6.6.0",
            r"C:\Program Files\Qt",
        ],
        "linux": [
            "/usr/lib/qt6",
            "/usr/lib/x86_64-linux-gnu/qt6",
            "/opt/qt6",
        ],
        "macos": [
            "/usr/local/opt/qt@6",
            "/opt/homebrew/opt/qt@6",
        ]
    }
    
    os_type = detect_platform()
    if os_type in qt_paths:
        for path in qt_paths[os_type]:
            if os.path.exists(path):
                print(f"   [OK] Qt6 found at: {path}")
                return True
    
    print("   [NOT FOUND] Qt6 is not installed")
    return False

def install_qt6_linux():
    """Install Qt6 on Linux"""
    print("\n[INSTALL] Installing Qt6 on Linux platform...")
    
    # Detect distro
    distro = None
    if os.path.exists("/etc/os-release"):
        with open("/etc/os-release") as f:
            for line in f:
                if line.startswith("ID="):
                    distro = line.split("=")[1].strip().strip('"')
                    break
    
    try:
        if distro in ["arch", "manjaro"]:
            print("   [INFO] Using pacman package manager")
            subprocess.run(["sudo", "pacman", "-S", "--noconfirm", 
                          "qt6-base", "qt6-multimedia", "cmake"], check=True)
        
        elif distro in ["ubuntu", "debian", "pop"]:
            print("   [INFO] Using apt package manager")
            subprocess.run(["sudo", "apt", "update"], check=True)
            subprocess.run(["sudo", "apt", "install", "-y",
                          "qt6-base-dev", "qt6-multimedia-dev", 
                          "libqt6multimedia6", "cmake", "build-essential"], 
                         check=True)
        
        elif distro == "fedora":
            print("   [INFO] Using dnf package manager")
            subprocess.run(["sudo", "dnf", "install", "-y",
                          "qt6-qtbase-devel", "qt6-qtmultimedia-devel", 
                          "cmake", "gcc-c++"], check=True)
        else:
            print(f"   [ERROR] Unsupported distribution: {distro}")
            return False
        
        print("   [SUCCESS] Qt6 installation completed")
        return True
    
    except subprocess.CalledProcessError as e:
        print(f"   [ERROR] Installation failed: {e}")
        return False

def install_qt6_windows():
    """Install Qt6 on Windows using Chocolatey"""
    print("\n[INSTALL] Installing Qt6 on Windows platform...")
    
    # Check if Chocolatey is installed
    if not shutil.which("choco"):
        print("   [INFO] Chocolatey not found, installing...")
        print("   [NOTICE] Administrator privileges required")
        
        ps_script = (
            "Set-ExecutionPolicy Bypass -Scope Process -Force; "
            "[System.Net.ServicePointManager]::SecurityProtocol = "
            "[System.Net.ServicePointManager]::SecurityProtocol -bor 3072; "
            "iex ((New-Object System.Net.WebClient).DownloadString("
            "'https://community.chocolatey.org/install.ps1'))"
        )
        
        try:
            subprocess.run(["powershell", "-Command", ps_script], 
                         check=True, shell=True)
        except:
            print("   [ERROR] Chocolatey installation failed")
            print("   [INFO] Manual installation: https://chocolatey.org/install")
            return False
    
    # Install Qt6
    try:
        print("   [INFO] Installing Qt6 via Chocolatey package manager")
        subprocess.run(["choco", "install", "qt6-base", "-y"], 
                      check=True, shell=True)
        print("   [SUCCESS] Qt6 installation completed")
        return True
    
    except subprocess.CalledProcessError:
        print("   [ERROR] Qt6 installation failed")
        print("   [INFO] Alternative: Download from https://www.qt.io/download")
        return False

def install_qt6_macos():
    """Install Qt6 on macOS using Homebrew"""
    print("\n[INSTALL] Installing Qt6 on macOS platform...")
    
    # Check if Homebrew is installed
    if not shutil.which("brew"):
        print("   [INFO] Homebrew not found, installing...")
        brew_install = (
            '/bin/bash -c "$(curl -fsSL '
            'https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
        )
        try:
            subprocess.run(brew_install, shell=True, check=True)
        except:
            print("   [ERROR] Homebrew installation failed")
            return False
    
    # Install Qt6
    try:
        print("   [INFO] Installing Qt6 via Homebrew package manager")
        subprocess.run(["brew", "install", "qt@6", "cmake"], check=True)
        print("   [SUCCESS] Qt6 installation completed")
        return True
    
    except subprocess.CalledProcessError:
        print("   [ERROR] Qt6 installation failed")
        return False

def build_project():
    """Build the project using CMake"""
    print("\n[BUILD] Initializing project build process...")
    
    project_root = Path(__file__).parent
    build_dir = project_root / "build"
    
    # Create build directory
    build_dir.mkdir(exist_ok=True)
    os.chdir(build_dir)
    
    try:
        # Run CMake configure
        print("   [CONFIG] Configuring project with CMake...")
        cmake_args = ["cmake", ".."]
        
        # Add generator for Windows
        if detect_platform() == "windows":
            cmake_args.extend(["-G", "Visual Studio 17 2022"])
        
        subprocess.run(cmake_args, check=True)
        
        # Run CMake build
        print("   [COMPILE] Compiling project sources...")
        build_args = ["cmake", "--build", ".", "--config", "Release"]
        
        # Add parallel jobs
        if detect_platform() != "windows":
            import multiprocessing
            jobs = multiprocessing.cpu_count()
            build_args.extend(["-j", str(jobs)])
        
        subprocess.run(build_args, check=True)
        
        print("   [SUCCESS] Build process completed successfully")
        return True
    
    except subprocess.CalledProcessError as e:
        print(f"   [ERROR] Build failed: {e}")
        return False

def print_usage_instructions():
    """Print how to run the executables"""
    print("\n" + "=" * 70)
    print("Setup Complete - Build Artifacts Ready")
    print("=" * 70)
    print("\n[INFO] Executable binaries location: build/bin/\n")
    
    os_type = detect_platform()
    
    if os_type == "windows":
        print("[SERVER] Starting the RTSP server:")
        print("   cd build\\bin")
        print("   .\\server.exe 8554")
        print("\n[CLIENT] Starting the video client:")
        print("   cd build\\bin")
        print("   .\\client.exe 127.0.0.1 8554 movie.Mjpeg 25000")
    else:
        print("[SERVER] Starting the RTSP server:")
        print("   cd build/bin")
        print("   ./server 8554")
        print("\n[CLIENT] Starting the video client:")
        print("   cd build/bin")
        print("   ./client 127.0.0.1 8554 movie.Mjpeg 25000")
    
    print("\n" + "=" * 70)

def main():
    """Main setup function"""
    print_banner()
    
    # Detect platform
    os_type = detect_platform()
    
    if os_type == "unknown":
        print(f"[ERROR] Unsupported operating system: {platform.system()}")
        sys.exit(1)
    
    print(f"[PLATFORM] Detected system: {os_type.upper()}\n")
    
    # Step 1: Check Qt6
    qt_installed = check_qt6_installed()
    
    # Step 2: Install Qt6 if needed
    if not qt_installed:
        print("\n[WARNING] Qt6 framework is required but not found.")
        response = input("Proceed with automatic Qt6 installation? (y/N): ")
        
        if response.lower() != 'y':
            print("[CANCELLED] Setup terminated. Please install Qt6 manually.")
            sys.exit(1)
        
        if os_type == "linux":
            if not install_qt6_linux():
                sys.exit(1)
        elif os_type == "windows":
            if not install_qt6_windows():
                sys.exit(1)
        elif os_type == "macos":
            if not install_qt6_macos():
                sys.exit(1)
    
    # Step 3: Build project
    print("\n" + "=" * 70)
    response = input("Proceed with project build? (Y/n): ")
    
    if response.lower() != 'n':
        if build_project():
            print_usage_instructions()
        else:
            print("\n[ERROR] Build process failed. Review error messages above.")
            sys.exit(1)
    else:
        print("\n[INFO] Setup completed. Execute this script again to build the project.")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n[CANCELLED] Setup interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n[ERROR] Unexpected error occurred: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

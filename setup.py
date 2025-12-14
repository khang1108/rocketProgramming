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
            # MSYS2 Qt6 (recommended)
            r"C:\msys64\mingw64\lib\cmake\Qt6",
            r"C:\msys32\mingw64\lib\cmake\Qt6",
            # Official Qt installer
            r"C:\Qt\6.8.0", r"C:\Qt\6.7.0", r"C:\Qt\6.6.0", r"C:\Qt\6.5.0",
            r"C:\Qt\6.4.0", r"C:\Qt\6.3.0", r"C:\Qt\6.2.0",
            r"C:\Program Files\Qt",
            r"C:\Qt6",
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
    """Install Qt6 on Windows using MSYS2"""
    print("\n[INSTALL] Installing Qt6 on Windows via MSYS2...")
    print("   [INFO] MSYS2 provides native Qt6 packages via pacman")
    
    # Check if MSYS2 is installed
    msys2_paths = [
        r"C:\msys64\usr\bin\pacman.exe",
        r"C:\msys32\usr\bin\pacman.exe",
        os.path.expanduser(r"~\msys64\usr\bin\pacman.exe"),
    ]
    
    pacman_path = None
    msys2_root = None
    
    for path in msys2_paths:
        if os.path.exists(path):
            pacman_path = path
            msys2_root = os.path.dirname(os.path.dirname(os.path.dirname(path)))
            break
    
    # Install MSYS2 if not found
    if not pacman_path:
        print("   [INFO] MSYS2 not found, installing...")
        print("   [STEP 1] Download MSYS2 installer:")
        print("            https://www.msys2.org/")
        print("            File: msys2-x86_64-latest.exe")
        print()
        print("   [STEP 2] Run installer with default settings")
        print("            Default path: C:\\msys64")
        print()
        print("   [STEP 3] After installation, MSYS2 terminal will open")
        print("            Run: pacman -Syu")
        print("            Close terminal when prompted")
        print()
        print("   [STEP 4] Re-run this setup script")
        print()
        
        response = input("   Open MSYS2 download page in browser? (y/N): ").strip().lower()
        if response == 'y':
            try:
                import webbrowser
                webbrowser.open('https://www.msys2.org/')
            except:
                pass
        
        return False
    
    print(f"   [OK] MSYS2 found at: {msys2_root}")
    
    # Update MSYS2 and install Qt6
    try:
        print("   [INFO] Updating MSYS2 packages...")
        
        # Set MSYS2 environment
        msys2_env = os.environ.copy()
        msys2_env['MSYSTEM'] = 'MINGW64'
        msys2_env['PATH'] = f"{msys2_root}\\mingw64\\bin;{msys2_root}\\usr\\bin;" + msys2_env['PATH']
        
        # Update package database
        subprocess.run([pacman_path, "-Sy", "--noconfirm"], 
                      env=msys2_env, check=True)
        
        print("   [INFO] Installing Qt6 and dependencies...")
        packages = [
            "mingw-w64-x86_64-qt6-base",
            "mingw-w64-x86_64-qt6-multimedia",
            "mingw-w64-x86_64-qt6-multimedia-wmf",
            "mingw-w64-x86_64-cmake",
            "mingw-w64-x86_64-ninja",
            "mingw-w64-x86_64-gcc",
        ]
        
        subprocess.run([pacman_path, "-S", "--noconfirm"] + packages,
                      env=msys2_env, check=True)
        
        print("   [SUCCESS] Qt6 installation completed via MSYS2")
        print()
        print("   [IMPORTANT] Add to Windows PATH:")
        print(f"            {msys2_root}\\mingw64\\bin")
        print()
        print("   How to add to PATH:")
        print("   1. Press Win + X → System → Advanced system settings")
        print("   2. Environment Variables → System Path → Edit → New")
        print("   3. Paste the path above")
        print()
        
        return True
        
    except subprocess.CalledProcessError as e:
        print(f"   [ERROR] Installation failed: {e}")
        print("   [INFO] Try manual installation:")
        print("           1. Open MSYS2 MINGW64 terminal")
        print("           2. Run: pacman -Syu")
        print("           3. Run: pacman -S mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-multimedia")
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
    
    if getattr(sys, 'frozen', False):
        executable_path = Path(sys.executable)
        project_root = executable_path.parent.parent
        print(f"   [INFO] Running from compiled binary: {executable_path}")
        print(f"   [INFO] Project root detected: {project_root}")
    else:
        project_root = Path(__file__).parent
        print(f"   [INFO] Running from Python script")
        print(f"   [INFO] Project root: {project_root}")
    
    build_dir = project_root / "build"
    
    cmake_file = project_root / "CMakeLists.txt"
    if not cmake_file.exists():
        print(f"   [ERROR] CMakeLists.txt not found at: {cmake_file}")
        print(f"   [ERROR] Please run this from the project root directory")
        return False
    
    build_dir.mkdir(exist_ok=True)
    os.chdir(build_dir)
    
    try:
        print("   [CONFIG] Configuring project with CMake...")
        cmake_args = ["cmake", str(project_root)]
        
        # Windows: Detect and use MSYS2 MinGW toolchain
        if detect_platform() == "windows":
            # Check for MSYS2 Qt6
            msys2_qt_paths = [
                r"C:\msys64\mingw64",
                r"C:\msys32\mingw64",
                os.path.expanduser(r"~\msys64\mingw64"),
            ]
            
            msys2_qt = None
            for path in msys2_qt_paths:
                if os.path.exists(os.path.join(path, "lib", "cmake", "Qt6")):
                    msys2_qt = path
                    break
            
            if msys2_qt:
                print(f"   [INFO] Using MSYS2 MinGW toolchain from: {msys2_qt}")
                cmake_args.extend([
                    "-G", "Ninja",
                    f"-DCMAKE_PREFIX_PATH={msys2_qt}",
                    f"-DCMAKE_C_COMPILER={msys2_qt}/bin/gcc.exe",
                    f"-DCMAKE_CXX_COMPILER={msys2_qt}/bin/g++.exe",
                ])
                
                # Add MSYS2 to PATH for this build
                msys2_bin = os.path.join(msys2_qt, "bin")
                os.environ['PATH'] = f"{msys2_bin};{os.environ.get('PATH', '')}"
            else:
                # Fallback to Visual Studio (if available)
                print("   [INFO] MSYS2 not found, attempting Visual Studio build...")
                cmake_args.extend(["-G", "Visual Studio 17 2022"])
        
        subprocess.run(cmake_args, check=True)
        
        print("   [COMPILE] Compiling project sources...")
        build_args = ["cmake", "--build", ".", "--config", "Release"]
        
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
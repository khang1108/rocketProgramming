# Windows Setup Guide - RTSP/RTP Video Streaming

## Quick Start (Recommended: MSYS2)

```bash
# Method 1: Run automated setup (installs via MSYS2)
python setup.py

# Method 2: Run batch file
setup_windows.bat
```

**✅ NEW:** We now use **MSYS2** for Windows - a native package manager with proper Qt6 support!

---

## Why MSYS2?

### ❌ Problems with traditional Windows Qt installation:
- **Qt.io Installer**: 3-5 GB download, manual component selection, interactive setup
- **Chocolatey**: Only provides installer launcher (`qt-sdk`), not actual packages
- **Winget/Scoop**: No Qt6 packages available

### ✅ MSYS2 Advantages:
- **Package Manager**: Like `apt` or `pacman` on Linux
- **Automated**: One command to install Qt6 + all dependencies
- **Lightweight**: ~500 MB (vs 3-5 GB for Qt.io installer)
- **Up-to-date**: Official Qt6 packages maintained by MSYS2 team
- **MinGW Toolchain**: GCC compiler included (no need for Visual Studio)

---

## Step-by-Step Installation Guide

### Step 1: Install MSYS2

#### 1.1 Download MSYS2 Installer
- Go to: **https://www.msys2.org/**
- Click "Download" → Get `msys2-x86_64-latest.exe`
- File size: ~90 MB

#### 1.2 Run Installer
- Run `msys2-x86_64-latest.exe`
- Install to default location: `C:\msys64`
- Click through installation (takes 2-3 minutes)

#### 1.3 Initial Update
After installation completes, MSYS2 terminal will open automatically:
```bash
# Update package database and core packages
pacman -Syu
```

When prompted `Proceed with installation? [Y/n]`, press `Y` and Enter.

**Important:** Terminal will close and ask you to restart. This is normal!

#### 1.4 Update Again (Important!)
- Open **MSYS2 MINGW64** from Start Menu (not MSYS2 MSYS)
- Run update again:
```bash
pacman -Su
```

### Step 2: Install Qt6 and Development Tools

In the **MSYS2 MINGW64** terminal, run:

```bash
# Install Qt6, CMake, Ninja, and GCC compiler
pacman -S mingw-w64-x86_64-qt6-base \
          mingw-w64-x86_64-qt6-multimedia \
          mingw-w64-x86_64-qt6-multimedia-wmf \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-ninja \
          mingw-w64-x86_64-gcc
```

When prompted, press `Y` to proceed.

**Installation size:** ~500 MB  
**Installation time:** 5-10 minutes

### Step 3: Add MSYS2 to Windows PATH

This allows you to use Qt6 from regular Windows Command Prompt/PowerShell.

#### 3.1 Find MSYS2 MinGW64 bin directory
Default: `C:\msys64\mingw64\bin`

#### 3.2 Add to PATH
1. Press `Win + X` → Select "System"
2. Click "Advanced system settings" (right sidebar)
3. Click "Environment Variables" button
4. Under "System variables", find and select "Path"
5. Click "Edit"
6. Click "New"
7. Add: `C:\msys64\mingw64\bin`
8. Click "OK" on all windows

#### 3.3 Verify Installation
**Close and reopen** Command Prompt/PowerShell, then run:
```powershell
qmake -v
# Should output: QMake version 3.1, Using Qt version 6.x.x

cmake --version
# Should output: cmake version 3.xx

g++ --version
# Should output: g++ (GCC) xx.x.x
```

If all commands work, you're ready to build!

---

### Step 2: Add Qt to System PATH

#### Method A: Automatic (if installer didn't do it)
Qt Installer should add path automatically. Verify by opening **Command Prompt**:
```cmd
qmake --version
```

If it works, skip to Step 3.

#### Method B: Manual PATH Configuration
1. Press `Win + X` → Select "System"
2. Click "Advanced system settings"
3. Click "Environment Variables"
4. Under "System variables", find and select "Path"
5. Click "Edit"
6. Click "New"
7. Add: `C:\Qt\6.x.x\msvc2019_64\bin`
   - Replace `6.x.x` with your actual version (e.g., `6.6.0`)
8. Click "OK" on all windows
9. **Restart Command Prompt** for changes to take effect

#### Verify PATH
```cmd
# Should show: QMake version 3.1, Using Qt version 6.x.x
qmake --version

# Should show Qt location
where qmake
```

---

### Step 4: Build the Project

#### 4.1 Open Terminal
You can use any of these:
- **Command Prompt** (press `Win + R`, type `cmd`)
- **PowerShell**
- **MSYS2 MINGW64** terminal

All work the same after adding MSYS2 to PATH!

#### 4.2 Navigate to Project
```bash
cd C:\path\to\rocketProgramming
# or
cd /c/path/to/rocketProgramming  # in MSYS2 terminal
```

#### 4.3 Build with CMake + Ninja
```bash
# Create and enter build directory
mkdir build
cd build

# Configure project (Ninja generator)
cmake .. -G Ninja

# Or let CMake auto-detect Qt from MSYS2
cmake ..

# Build (parallel jobs for speed)
cmake --build . -j 8

# Or use ninja directly
ninja
```

**Build time:** 2-5 minutes (first time)

#### 4.4 Run the Programs

**Server:**
```bash
.\bin\server.exe 8554 ..\video\movie.Mjpeg

# Or from project root:
.\build\bin\server.exe 8554 video\movie.Mjpeg
```

**Client** (in another terminal):
```bash
.\bin\client.exe localhost 8554 movie.Mjpeg 25000

# Or from project root:
.\build\bin\client.exe localhost 8554 movie.Mjpeg 25000
```

#### 4.5 (Optional) Install System-wide
```bash
# In build directory
cmake --install . --prefix C:\Program Files\RocketProgramming
```

---

## Troubleshooting

### Error: "Qt6 not found" by CMake
**Solution 1:** Specify MSYS2 Qt path:
```bash
cmake .. -DCMAKE_PREFIX_PATH="C:/msys64/mingw64"
```

**Solution 2:** Make sure you're using MSYS2 MINGW64 terminal:
- Not "MSYS2 MSYS" or "MSYS2 UCRT64"
- Use "MSYS2 MINGW64" specifically

### Error: "qmake not found" or "cmake not found"
**Solution:** MSYS2 bin not in PATH.

**Temporary fix** (this session only):
```bash
export PATH="/c/msys64/mingw64/bin:$PATH"
```

**Permanent fix:** Add to Windows PATH (see Step 3.2)

### Error: "Could not find Qt6Multimedia"
**Solution:** Install the multimedia package:
```bash
pacman -S mingw-w64-x86_64-qt6-multimedia
```

### Error: Missing DLLs when running (libgcc, libstdc++, Qt6Core.dll)
**Solution:** DLL search path issue.

**Option A:** Run from MSYS2 terminal:
```bash
cd build/bin
./client.exe localhost 8554 movie.Mjpeg 25000
```

**Option B:** Copy DLLs to exe directory:
```bash
# From build directory
ldd bin/client.exe  # Shows missing DLLs
cp /mingw64/bin/*.dll bin/  # Copy all MSYS2 DLLs
```

**Option C:** Add MSYS2 to PATH permanently (recommended - Step 3.2)

### Error: "Ninja not found"
**Solution:** Install Ninja or use default generator:
```bash
# Install Ninja
pacman -S mingw-w64-x86_64-ninja

# Or use default Makefiles
cmake .. -G "MinGW Makefiles"
cmake --build .
```

### MSYS2 pacman errors: "failed to synchronize any databases"
**Solution:** Update pacman mirrors:
```bash
# Update mirror list
pacman -Sy pacman-mirrors

# Then update system
pacman -Syu
```

### CMake uses wrong compiler (MSVC instead of GCC)
**Solution:** Force GCC compiler:
```bash
cmake .. -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
```

### Build fails with "undefined reference to WinMain"
**Solution:** Make sure you're linking correctly. Check CMakeLists.txt has:
```cmake
add_executable(client WIN32 ...)  # For GUI
# or
add_executable(server ...)  # For console
```

---

## MSYS2 vs Qt Official Installer

| Feature | MSYS2 (Recommended) | Qt.io Installer |
|---------|---------------------|-----------------|
| **Installation Size** | ~500 MB | 3-5 GB |
| **Installation Time** | 5-10 minutes | 20-40 minutes |
| **Automation** | ✅ Fully automated (`pacman -S ...`) | ❌ Manual component selection |
| **Updates** | ✅ `pacman -Syu` | ⚠️ Qt Maintenance Tool (slow) |
| **Compiler** | ✅ GCC/MinGW (included) | ❌ Requires Visual Studio (7+ GB) |
| **CMake** | ✅ Included | ⚠️ Optional component |
| **Package Manager** | ✅ Yes (pacman) | ❌ No |
| **Cross-platform** | ✅ Similar to Linux | ❌ Windows-specific |
| **Qt Account** | ✅ Not required | ❌ Required |

**Recommendation:** Use MSYS2 unless you specifically need:
- Qt Quick/QML Designer
- Commercial Qt license features
- MSVC-specific features

## Alternative: Use Pre-built Binaries

If manual setup is too complex, check **GitHub Releases** for pre-compiled executables:
- https://github.com/YOUR_REPO/releases
- Download `rocketProgramming-windows.zip`
- Extract and run directly (no build needed)

---

## Summary Checklist

- [x] MSYS2 installed (`C:\msys64`)
- [x] Qt6 packages installed via pacman
- [x] MSYS2 mingw64/bin added to Windows PATH
- [x] `qmake -v`, `cmake --version`, `g++ --version` all work
- [x] Built project with CMake + Ninja
- [x] Executables run successfully

---

## Need Help?

- **Qt Documentation:** https://doc.qt.io/qt-6/windows.html
- **CMake Documentation:** https://cmake.org/cmake/help/latest/
- **Visual Studio:** https://docs.microsoft.com/en-us/cpp/

**Common Issues:**
- 90% of problems: Not using "Developer Command Prompt"
- 10% of problems: Missing Qt Multimedia component

**Support:**
- Create an issue on GitHub with full error log
- Include: Qt version, VS version, CMake output

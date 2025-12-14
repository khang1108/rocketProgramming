# Quick Start Guide for Windows (5 Minutes)

## TL;DR - Fastest Setup

### 1. Install MSYS2 (2 minutes)
- Download: https://www.msys2.org/
- Run `msys2-x86_64-latest.exe`
- Install to `C:\msys64` (default)

### 2. Update MSYS2 (1 minute)
Terminal will open after install:
```bash
pacman -Syu
# Terminal will close - this is normal!
```

Open **MSYS2 MINGW64** from Start Menu:
```bash
pacman -Su
```

### 3. Install Qt6 + Tools (2 minutes)
```bash
pacman -S mingw-w64-x86_64-qt6-base \
          mingw-w64-x86_64-qt6-multimedia \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-ninja \
          mingw-w64-x86_64-gcc
```

Press `Y` when prompted.

### 4. Add to PATH
1. Press `Win + X` → System → Advanced → Environment Variables
2. Edit "Path" → New → Add: `C:\msys64\mingw64\bin`
3. Click OK

### 5. Build Project
Open Command Prompt:
```bash
cd path\to\rocketProgramming
mkdir build && cd build
cmake .. -G Ninja
ninja
```

### 6. Run
```bash
# Server
.\bin\server.exe 8554 ..\video\movie.Mjpeg

# Client (new terminal)
.\bin\client.exe localhost 8554 movie.Mjpeg 25000
```

---

## Even Faster: Automated Setup

```bash
# Just run this:
python setup.py
```

The script will:
1. Check if MSYS2 is installed
2. Guide you through installation if needed
3. Install Qt6 automatically via pacman
4. Build the project

---

## Verify Installation

```bash
qmake -v      # Should show Qt 6.x.x
cmake --version
g++ --version
```

All three should work. If not, see [SETUP_WINDOWS.md](SETUP_WINDOWS.md) for troubleshooting.

---

## Why MSYS2?

- ✅ **500 MB** (vs 3-5 GB for Qt.io installer)
- ✅ **One command** to install all dependencies
- ✅ **No Qt account** required
- ✅ **Package manager** like Linux (pacman)
- ✅ **Updates easy**: `pacman -Syu`

## Need More Details?

See full guide: [SETUP_WINDOWS.md](SETUP_WINDOWS.md)


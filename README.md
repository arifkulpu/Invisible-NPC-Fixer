# Invisible-NPC-Fixer

An SKSE plugin for Skyrim Anniversary Edition (1.6.1170) that automatically detects and resolves "invisible NPC" and "naked NPC" glitches by forcing a 3D model refresh.

## Features
- Detects NPCs in the active cell that have failed to render their 3D models.
- Automatically triggers a `Disable/Enable` cycle or 3D refresh to force the game engine to reload the actor.
- Lightweight and performance-conscious Main Loop hook.

## Requirements
- Skyrim Special Edition / Anniversary Edition (1.6.1170)
- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

## Installation
1. Install requirements.
2. Build the `.dll` or download from releases.
3. Place `InvisibleNPCFixer.dll` in `Data/SKSE/Plugins/`.

## Building
This project uses **CMake** and **vcpkg** for dependency management.

1. Clone the repository:
   ```bash
   git clone https://github.com/arifkulpu/Invisible-NPC-Fixer.git
   ```
2. Set up vcpkg and install dependencies:
   ```bash
   vcpkg install commonlibsse-ng
   ```
3. Generate project files:
   ```bash
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
   ```
4. Build:
   ```bash
   cmake --build build --config Release
   ```

## License
MIT License

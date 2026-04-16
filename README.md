# Invisible-NPC-Fixer

An SKSE plugin for Skyrim Special Edition & Anniversary Edition (1.5.97 - 1.6.1170+) that automatically detects and resolves "invisible NPC" and "naked NPC" glitches by forcing a 3D model refresh.

## Features & Optimizations
- **Smart Detection:** Detects NPCs in the active cell that have failed to render their 3D models.
- **Engine-Safe Refresh:** Automatically triggers a 3D refresh (`Update3DModel`) to force the game engine to reload the actor. This is safely queued using the SKSE Task Interface to prevent mid-render CTDs (Crash to Desktop).
- **High Performance:** 
  - Uses `a_delta` frametime calculations instead of heavy OS clocks (`std::chrono`) for internal timers.
  - Implements $O(1)$ Hash-Tables (`std::unordered_map`) for extremely fast memory tracking of already fixed NPCs.
  - Limits distance checks (approx. 4096 units radius) to only process NPCs that are actually near the player, saving valuable CPU cycles.
- **Stability Checks:** Ignores dead, disabled, or pending deletion (`IsDeleted`) actors to prevent engine instability and random crashes.
- **Next-Gen Compatible:** Fully integrates `CommonLibSSE-NG` and `SKSEPluginLoad` architecture, providing seamless Address Library backwards/forwards compatibility across both 1.5.97 and 1.6.xx versions.

## Requirements
- Skyrim Special Edition / Anniversary Edition 
- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

## Installation
1. Install requirements.
2. Build the `.dll` or download from releases.
3. Place `NpcGhostFix.dll` in `Data/SKSE/Plugins/`.

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

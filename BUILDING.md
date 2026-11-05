# Modern Windows renderer

The Direct3D 12 renderer is an independent starter target. The original
Visual Studio 2010 / Direct3D 8 application remains unchanged while rendering
features are migrated incrementally.

## Requirements

- Windows 10 or later
- Visual Studio 2022 with the **Desktop development with C++** workload
- A Windows 10/11 SDK
- CMake 3.25 or later

## Configure and build

From a Visual Studio Developer PowerShell in the repository root:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-vs2022-debug
```

For an optimized build, use `windows-vs2022-release` as the build preset.
The executable is written under `out/build/windows-vs2022/renderer/d3d12/`.

The first target opens a resizable window and clears it using Direct3D 12. It
provides a place to validate the Windows SDK, DXGI swap chain, command queue,
frame synchronization, and resize path before porting the existing renderer.
It does not yet load RailSim2 assets or replace the existing application.

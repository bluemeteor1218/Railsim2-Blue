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

## D3DX math compatibility layer

`renderer/compat/d3dx_math.h` provides the D3DX8 vector, matrix, quaternion,
plane and bounding-probe API on top of DirectXMath, so legacy code can drop
`d3dx8.h` for math. Run its tests with:

```powershell
cmake --build out/build/windows-vs2022 --config Debug
ctest --test-dir out/build/windows-vs2022 -C Debug --output-on-failure
```

If `cmake` is not on PATH (Build Tools only), call it from
`C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`.

## レンダラー構成

- renderer/core: D3D非依存のコア (RenderState, ViewportLayout, SpriteBatch, TextLayout, image_ops)。ctestで検証。
- renderer/compat: D3DX数学互換レイヤー。
- renderer/d3d12: D3D12実装 (pipeline, texture, font_atlas, ui_renderer, main)。
- テスト: ctest --test-dir out\build\windows-vs2022 -C Debug --output-on-failure


## 画面サイズ

- 起動オプション: railsim2_d3d12_renderer.exe --size 1920x1080 (既定は 1280x720)
- キー: 1 = 640x480 / 2 = 1280x720 / 3 = 1920x1080 / F11 = ボーダーレス全画面切替
- 要求サイズがモニターの作業領域に収まらない場合 (1080p画面での1920x1080など) は、自動でボーダーレス全画面になります。
- ウィンドウを自由にリサイズしても、UIは高さ480基準で拡大されます。


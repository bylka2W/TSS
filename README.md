# TSS - Temporal Super Sampling for Unreal Engine 5.3

A custom temporal upscaling plugin for UE 5.3, replicating AMD FSR2's full 6-pass architecture with compute shaders.

## Architecture

Based on [AMD FidelityFX FSR2](https://github.com/GPUOpen-Effects/FidelityFX-FSR2), the pipeline consists of 6 compute shader passes:

| Pass | Shader | Description |
|------|--------|-------------|
| 1 | `TSSLuminancePyramid` | Auto-exposure from luminance average |
| 2 | `TSSReconstructDilate` | 3x3 nearest-depth dilation for depth + motion vectors |
| 3 | `TSSDepthClip` | Disocclusion mask + RGB-to-YCoCg conversion |
| 4 | `TSSCreateLocks` | Pixel lock mask from luminance range |
| 5 | `TSSReprojectAccumulate` | Temporal resolve with neighbor clamping + history feedback |
| 6 | `TSSRCAS` | Robust Contrast Adaptive Sharpening |

## Pipeline Flow

```
SceneColor ───────────────────────────────────────────────────────────────────────┐
SceneDepth ──┐                                                                     │
             ├──► ReconstructDilate ──► DilatedDepth ──┐                           │
MotionVecs ──┘                          DilatedMV ─────┤                           │
                                                       ├──► DepthClip ──► AdjustedColor ──┐
Exposure (1x1) ◄── LuminancePyramid                     │                  ReactiveMask     │
                                                         │                                  │
PrevDepth (history) ─────────────────────────────────────┘                                  │
                                                                                           │
LockMask ◄── CreateLocks ◄── AdjustedColor                                                  │
                                                                                           │
HistoryColor (history) ──┐                                                                 │
                         ├──► ReprojectAccumulate ──► UpscaledBuffer ──► RCAS ──► Final ──► BackBuffer
AdjustedColor ───────────┘        ▲                                                        │
DilatedMV ────────────────────────┘                                                        │
ReactiveMask ──────────────────────────────────────────────────────────────────────────────┘
```

## Integration

The plugin integrates into UE 5.3's rendering pipeline via `FSceneViewExtensionBase`:

- **`PrePostProcessPass_RenderThread`** — Extracts SceneColor, SceneDepth, and MotionVectors from the scene uniform buffer
- **`PostRenderViewFamily_RenderThread`** — Executes the full 6-pass pipeline and copies the result to the back buffer

### Key Integration Points

```
Engine Rendering Pass
    │
    ├──► PrePostProcessPass_RenderThread (extract textures)
    │
    ├──► PostProcessing (bloom, tonemapping, etc.)
    │
    └──► PostRenderViewFamily_RenderThread (run TSS pipeline → back buffer)
```

## Controls

| Input | Action |
|-------|--------|
| **F** (in-game) | Toggle TSS on/off |
| `tss.Enable 0/1` | Console: disable/enable |
| `tss.Toggle` | Console: toggle |
| `r.ScreenPercentage N` | Set render resolution percentage |

## Requirements

- Unreal Engine 5.3
- Windows (DX11/DX12)
- Compute Shader support (CS 5.0+)

## File Structure

```
TSS/
├── TSS.uplugin
├── .gitignore
├── README.md
├── Resources/
│   └── Icon128.png
├── Shaders/Private/
│   ├── TSSCommon.ush              — Shared HLSL utilities (luminance, YCoCg)
│   ├── TSSLuminancePyramid.usf    — Pass 1: Auto-exposure
│   ├── TSSReconstructDilate.usf   — Pass 2: Depth/MV dilation
│   ├── TSSDepthClip.usf           — Pass 3: Disocclusion + color adjust
│   ├── TSSCreateLocks.usf         — Pass 4: Pixel lock mask
│   ├── TSSReprojectAccumulate.usf — Pass 5: Temporal resolve
│   ├── TSSRCAS.usf                — Pass 6: Contrast-adaptive sharpening
│   ├── TSSUpscale.usf             — Bilinear upscale (test/debug)
│   └── TSSPassthrough.usf         — Passthrough (test/debug)
└── Source/TSS/
    ├── TSS.Build.cs
    ├── Public/
    │   ├── TSS.h                   — Module header, GTSSActive toggle
    │   ├── TSSViewExtension.h      — View extension with scene texture extraction
    │   └── TSSContext.h            — (Reserved for future context management)
    └── Private/
        ├── TSS.cpp                 — Module startup, F1 key binding, CVar registration
        ├── TSSViewExtension.cpp    — PrePostProcess extraction + PostRender pipeline dispatch
        ├── TSSPipeline.h           — Pipeline state (history buffers) + inputs
        ├── TSSPipeline.cpp         — Full 6-pass pipeline orchestration
        ├── TSSUpscalePass.h/.cpp   — Bilinear upscale compute pass
        ├── TSSShaders.h            — All shader class declarations (C++)
        └── TSSShaderImplementations.cpp — IMPLEMENT_GLOBAL_SHADER registrations
```

## Technical Details

### Two-Phase Rendering

The plugin uses a two-phase approach because `PrePostProcessPass_RenderThread` fires before post-processing, while `PostRenderViewFamily_RenderThread` fires after:

1. **Phase 1** (`PrePostProcessPass`): Saves `FRDGTextureRef` pointers to SceneColor, SceneDepth, and MotionVectors from the scene uniform buffer
2. **Phase 2** (`PostRenderViewFamily`): Uses saved textures to run the full pipeline and write output to back buffer

Both phases share the same `FRDGBuilder`, so texture references are valid across phases without extraction.

### UE 5.3 Specifics

- `FPostProcessingInputs` is forward-declared in `SceneViewExtension.h`; requires struct mirror to access fields
- `FSceneTextureUniformParameters` members: `SceneColorTexture`, `SceneDepthTexture`, `GBufferVelocityTexture`
- `SHADER_USE_PARAMETER_STRUCT` only handles C++ side binding; `.usf` files must declare their own HLSL parameters
- `AddCopyTexturePass` requires identical formats between source and destination
- History buffers persist via `QueueTextureExtraction` → `IPooledRenderTarget` across frames

### Shader Parameters (UE 5.3)

Engine shaders use `#include "/Engine/Private/Common.ush"` (NOT `/Engine/Public/Platform.ush`).

## License

MIT

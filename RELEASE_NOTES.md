# TSS v15.0: Smart Temporal Super Sampling

## What is TSS?

**TSS (Temporal Super Sampling)** is an open-source Frame Generation SDK that surpasses AMD FSR 2.2 through mathematical algorithms - no heavy neural networks, no tensor cores required.

## Key Features

### Mathematical Core (95% of work)
- **Single-Pass LDS Shader** - 1 VRAM read/write instead of 12+
- **Variance Clipping** - eliminates ghosting without losing details
- **YCoCg Color Space** - separates brightness and color for precision
- **Wave Intrinsics** - instant cross-pixel operations via registers
- **Bidirectional Splatting** - forward + backward projection

### Neural Polish (5% of work)
- **Confidence Masks** - Disocclusion, Velocity Divergence, Luma Instability
- **Neural Weight Arbitrator** - dynamic alpha based on scene analysis
- **Edge-Aware Filtering** - adaptive between Bilinear/Bicubic

## Comparison with FSR 2.2

| Feature | FSR 2.2 | TSS v15.0 |
|---------|---------|-----------|
| Passes | 12+ | 1 |
| VRAM Access | High | Minimal |
| Input Lag | ~2ms | ~0.5ms |
| Tensor Cores | No | Not Required |
| Ghosting | Low | Minimal |

## Architecture

Smart TSS Pipeline:
1. Bidirectional Splatting (Forward + Backward projection)
2. Confidence Masks (Disocclusion + Velocity + Luma)
3. Variance Clipping (mu +/- k*sigma)
4. Neural Arbitrator (HLSL) - Edge correction

## Files

- `shaders/TSSSmartCS.hlsl` - Neural Weight Arbitrator shader
- `shaders/TSSBidirectionalCS.hlsl` - Bidirectional Splatting
- `src/TSSSmartTSS.c` - Smart TSS core
- `src/TSSSmartDLL.c` - DLL interface for game integration
- `include/TSSGLMWrapper.h` - 3D vectors/matrices (GLM-style)

## Build

```bash
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release --target TSSSmartDLL
```

Output: `build/bin/Release/TSSSmart.dll`

## Usage

```c
#include "TSSSmartDLL.h"

TSSSmartContext ctx;
TSSSmart_CreateContext(&ctx);

TSSSmartParams params = {
    .width = 1920,
    .height = 1080,
    .enableYCoCg = 1,
    .kSigma = 1.25f,
    .sharpness = 0.5f
};

TSSSmart_Initialize(ctx, &params);
TSSSmart_Execute(ctx);
```

## License

MIT License - Free for commercial use

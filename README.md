# TSS — Texture Scaling Suite

Upscaling filters (EASU, Bilinear, RCAS) ported from FSR1 to a B⁺→HLSL→DX12 compute pipeline.

## Overview

- **B⁺ source** → `src/tss_shader.b+` — EASU, Bilinear, RCAS in one file
- **HLSL** → `src/tss_shader.hlsl` (generated via `bpc hlsl`)
- **DX12 compute** → `src/dx12_test.cpp` — dispatches shader, writes PPM and raw float32
- **Validation** → `src/validate_easu.c` — compares DX12 output against CPU reference

## Results

| Metric    | Value          |
|-----------|----------------|
| PSNR      | 145.73 dB      |
| MaxErr    | 0.000001       |
| Bit-match | ≈100% (modulo <1 ulp) |

## Dependencies

- [B⁺ Compiler](https://github.com/bylka2W/B-Plus)
- DXC (`dxc.exe`)
- Windows 10 SDK 10.0.26100.0
- MSVC 14.30
- DirectX 12 GPU (tested on RTX 5060 Ti)

## Build

```cmd
bpc hlsl src\tss_shader.b+ -o src\tss_shader.hlsl
dxc -T cs_6_0 -E TSS_EASU src\tss_shader.hlsl -Fo src\tss_easu.cso
cl /O2 /EHsc src\dx12_test.cpp
cl /O2 src\validate_easu.c
```

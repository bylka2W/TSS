TSS API Reference
=================

TSS_GetVersion()
----------------
Returns SDK version.

```c
TSS_Version ver = TSS_GetVersion();
printf("TSS v%u.%u.%u\n", ver.major, ver.minor, ver.patch);
```

TSS_GetScratchMemorySize()
--------------------------
Get required scratch buffer size.

```c
size_t size;
TSS_GetScratchMemorySize(3840, 2160, &size);
```

TSS_CreateContext()
-------------------
Create frame generation context.

```c
TSS_ContextHandle ctx;
void* scratch = malloc(scratchSize);

TSS_ContextDesc desc = {};
desc.displayWidth = 1920;
desc.displayHeight = 1080;
desc.maxWidth = 3840;
desc.maxHeight = 2160;
desc.flags = TSS_FLAG_ANTI_GHOSTING;
desc.sharpness = 0.8f;

TSS_CreateContext(&desc, scratch, scratchSize, &ctx);
```

TSS_Dispatch()
--------------
Process a frame and generate upscaled output.

```c
TSS_DispatchDesc dispatch = {};
dispatch.color = colorBuffer;
dispatch.colorWidth = 1280;
dispatch.colorHeight = 720;
dispatch.motionVectors = mvBuffer;
dispatch.depth = depthBuffer;
dispatch.frameTimeDelta = 1.0f / 60.0f;

TSS_OutputDesc output = {};
TSS_Dispatch(ctx, &dispatch, &output);
```

C++ Interface
-------------
```cpp
TSS::Context ctx;
TSS::ContextDesc desc;
desc.setQuality(TSS_QUALITY_MODE_QUALITY);

ctx.initialize(desc);

TSS::DispatchDesc frame;
frame.color = colorBuffer;
ctx.dispatch(frame, output);
```

Flags
-----
- `TSS_FLAG_ANTI_GHOSTING` - Enable anti-ghosting
- `TSS_FLAG_HDR_OUTPUT` - HDR output support
- `TSS_FLAG_FAST_MODE` - Faster processing
- `TSS_FLAG_AUTO_EXPOSURE` - Automatic exposure

Sharpness
---------
Sharpness range: 0.0 to 1.0
- 0.0 = Maximum smoothness
- 1.0 = Maximum sharpness

```cpp
desc.sharpness = 0.8f; // Default
```

Jitter
-----
Apply sub-pixel jitter to rendering:

```cpp
float jitterX, jitterY;
TSS_GetJitterOffset(frameCount, &jitterX, &jitterY);

// Apply to projection matrix
projectionMatrix[2][0] += jitterX / renderWidth;
projectionMatrix[2][1] += jitterY / renderHeight;
```

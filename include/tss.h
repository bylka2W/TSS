#ifndef TSS_H
#define TSS_H

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_VERSION "1.0.0"
#define TSS_API __declspec(dllexport)

typedef enum {
    TSS_OK = 0,
    TSS_ERROR_INVALID_PARAM = -1,
    TSS_ERROR_NO_MEMORY = -2,
    TSS_ERROR_NO_DEVICE = -3,
    TSS_ERROR_NO_SHADER = -4
} TSSResult;

typedef enum {
    TSS_MODE_PERFORMANCE,
    TSS_MODE_BALANCED,
    TSS_MODE_QUALITY,
    TSS_MODE_ULTRA
} TSSMode;

typedef enum {
    TSS_FORMAT_R8G8B8A8_UNORM,
    TSS_FORMAT_R16G16B16A16_FLOAT,
    TSS_FORMAT_R32G32B32A32_FLOAT
} TSSFormat;

typedef struct {
    TSSMode mode;
    TSSFormat format;
    int renderWidth;
    int renderHeight;
    int displayWidth;
    int displayHeight;
    float sharpness;
    float kSigma;
    float motionScale;
    float disocclusionThreshold;
    int enableYCoCg;
    int enableNeuralRepair;
    int enableAdaptiveClamp;
    int enableDilatedMV;
    int enableAsyncCompute;
    int enableJitterStabilization;
    int maxFPS;
    int vsync;
} TSSDispatchDesc;

typedef struct {
    TSSResult result;
    float inputLag_ms;
    float gpuTime_ms;
    float totalTime_ms;
    int framesProcessed;
    int disocclusionsDetected;
    int ghostingSuppressions;
    float avgConfidence;
    float fps;
    int vrwmReads;
    int vrwmWrites;
} TSSPerformanceStats;

typedef void* TSSContext;

TSS_API const char* TSS_GetVersion(void);

TSS_API TSSContext TSS_CreateContext(void);

TSS_API TSSResult TSS_DestroyContext(TSSContext context);

TSS_API TSSResult TSS_Initialize(TSSContext context, TSSDispatchDesc* desc);

TSS_API TSSResult TSS_Shutdown(TSSContext context);

TSS_API TSSResult TSS_Dispatch(
    TSSContext context,
    void* currentColor,
    void* currentDepth,
    void* currentMotionVector,
    void* historyColor,
    void* historyDepth,
    void* historyMotionVector,
    void* outputColor
);

TSS_API TSSResult TSS_SetConstants(
    TSSContext context,
    float sharpness,
    float kSigma,
    float motionScale
);

TSS_API TSSResult TSS_GetStats(TSSContext context, TSSPerformanceStats* stats);

TSS_API TSSResult TSS_ResetStats(TSSContext context);

TSS_API TSSResult TSS_EnableDebug(TSSContext context, int enable);

typedef void (*TSSDebugCallback)(const char* message, int level);
TSS_API void TSS_SetDebugCallback(TSSDebugCallback callback);

TSS_API TSSResult TSS_ValidateHardware(void);

TSS_API const char* TSS_GetLastError(TSSContext context);

#ifdef __cplusplus
}
#endif

#endif

#ifndef TSS_HYBRID_WRAPPER_H
#define TSS_HYBRID_WRAPPER_H

#ifdef TSS_EXPORTS
#define TSS_API __declspec(dllexport)
#else
#define TSS_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_HYBRID_VERSION "1.0.0"
#define TSS_OVERRIDE_NAME "TSSHybrid"

typedef enum {
    TSS_HOOK_OK = 0,
    TSS_HOOK_ERROR_NOT_FOUND = -1,
    TSS_HOOK_ERROR_ALREADY_HOOKED = -2,
    TSS_HOOK_ERROR_INIT_FAILED = -3,
    TSS_HOOK_ERROR_NO_D3D12 = -4
} TSSHookResult;

typedef enum {
    TSS_MODE_FSR_REPLACEMENT,
    TSS_MODE_FSR_ENHANCER,
    TSS_MODE_TSS_NATIVE
} TSSMode;

typedef struct {
    TSSMode mode;
    int width;
    int height;
    float sharpness;
    float kSigma;
    float repairStrength;
    float jitterStabilization;
    float disocclusionThreshold;
    float confidenceThreshold;
    int enableYCoCg;
    int enableDepthTest;
    int enableLumaRepair;
    int enableAsyncCompute;
    int enableDebug;
    int targetFPS;
    int vsync;
} TSSHybridConfig;

TSS_API const char* TSSHybrid_GetVersion(void);

TSS_API TSSHookResult TSSHybrid_Initialize(void);

TSS_API void TSSHybrid_Shutdown(void);

TSS_API TSSHookResult TSSHybrid_SetConfig(TSSHybridConfig* config);

TSS_API TSSHookResult TSSHybrid_GetConfig(TSSHybridConfig* config);

TSS_API TSSHookResult TSSHybrid_EnableHook(void);

TSS_API TSSHookResult TSSHybrid_DisableHook(void);

TSS_API int TSSHybrid_IsHooked(void);

TSS_API TSSHookResult TSSHybrid_OverrideFSR(void);

TSS_API TSSHookResult TSSHybrid_RestoreFSR(void);

TSS_API TSSHookResult TSSHybrid_InjectFrame(
    void* colorTexture,
    void* depthTexture,
    void* motionVectorTexture,
    void* outputTexture
);

TSS_API void TSSHybrid_SetDebugCallback(void (*callback)(const char* message));

typedef struct {
    float inputLag_ms;
    float gpuTime_ms;
    float totalTime_ms;
    int framesProcessed;
    int disocclusionsDetected;
    int jitterFixesApplied;
    int ghostingSuppressions;
    float avgConfidence;
    float fps;
} TSSHybridStats;

TSS_API void TSSHybrid_GetStats(TSSHybridStats* stats);

TSS_API void TSSHybrid_ResetStats(void);

#ifdef __cplusplus
}
#endif

#endif

#ifndef TSS_SMART_DLL_H
#define TSS_SMART_DLL_H

#ifdef TSS_EXPORTS
#define TSS_API __declspec(dllexport)
#else
#define TSS_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_SMART_DLL_VERSION "1.0.0"

typedef void* TSSSmartContext;

typedef enum {
    TSS_SMART_OK = 0,
    TSS_SMART_ERROR_INVALID_PARAM = -1,
    TSS_SMART_ERROR_NO_MEMORY = -2,
    TSS_SMART_ERROR_NO_DEVICE = -3,
    TSS_SMART_ERROR_NO_SHADER = -4,
    TSS_SMART_ERROR_ALREADY_INITIALIZED = -5,
    TSS_SMART_ERROR_NOT_INITIALIZED = -6
} TSSSmartResult;

typedef struct {
    int width;
    int height;
    int historyWidth;
    int historyHeight;
    int frameIndex;
    int enableYCoCg;
    int enableDepthTest;
    int enableAsyncCompute;
    int enableNegativeLOD;
    float jitterX;
    float jitterY;
    float sharpness;
    float kSigma;
    float motionScale;
    float disocclusionThreshold;
    float maxVelocity;
    float deltaTime;
} TSSSmartParams;

typedef struct {
    TSSSmartResult result;
    float inputLag_ms;
    float gpuTime_ms;
    float totalTime_ms;
    int passesExecuted;
    int vrwmReads;
    int vrwmWrites;
    float confidenceAvg;
    float disocclusionRate;
} TSSSmartPerformance;

TSS_API const char* TSSSmart_GetVersion(void);

TSS_API TSSSmartResult TSSSmart_CreateContext(TSSSmartContext* context);

TSS_API TSSSmartResult TSSSmart_DestroyContext(TSSSmartContext context);

TSS_API TSSSmartResult TSSSmart_Initialize(
    TSSSmartContext context,
    TSSSmartParams* params
);

TSS_API TSSSmartResult TSSSmart_Shutdown(TSSSmartContext context);

TSS_API TSSSmartResult TSSSmart_SetCurrentFrame(
    TSSSmartContext context,
    void* colorTexture,
    void* depthTexture,
    void* motionVectorTexture
);

TSS_API TSSSmartResult TSSSmart_SetHistoryFrame(
    TSSSmartContext context,
    void* colorTexture,
    void* depthTexture,
    void* motionVectorTexture,
    void* confidenceTexture
);

TSS_API TSSSmartResult TSSSmart_Execute(TSSSmartContext context);

TSS_API TSSSmartResult TSSSmart_GetOutput(
    TSSSmartContext context,
    void** colorTexture,
    void** motionVectorTexture,
    void** confidenceTexture
);

TSS_API TSSSmartResult TSSSmart_GetPerformance(
    TSSSmartContext context,
    TSSSmartPerformance* performance
);

TSS_API TSSSmartResult TSSSmart_SetConfig(
    TSSSmartContext context,
    TSSSmartParams* params
);

TSS_API TSSSmartResult TSSSmart_GetConfig(
    TSSSmartContext context,
    TSSSmartParams* params
);

TSS_API TSSSmartResult TSSSmart_BeginAsync(TSSSmartContext context);

TSS_API TSSSmartResult TSSSmart_EndAsync(TSSSmartContext context);

typedef TSSSmartResult (*TSSSmart_LogCallback)(const char* message, int level);

TSS_API void TSSSmart_SetLogCallback(TSSSmart_LogCallback callback);

TSS_API const char* TSSSmart_GetLastError(TSSSmartContext context);

#ifdef __cplusplus
}
#endif

#endif

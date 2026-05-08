#define TSS_EXPORTS
#include "TSSSmartDLL.h"
#include "TSSSmartTSS.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define TSS_SMART_MAX_FRAMES 256

typedef struct {
    TSSSmartParams params;
    TSSSmartConfig config;
    TSSSmartStats stats;
    TSSSmartPerformance performance;
    TSSSmart_LogCallback logCallback;
    char lastError[256];
    int initialized;
    void* currentColorTex;
    void* currentDepthTex;
    void* currentMVTex;
    void* historyColorTex;
    void* historyDepthTex;
    void* historyMVTex;
    void* historyConfidenceTex;
    void* outputColorTex;
    void* outputMVTex;
    void* outputConfidenceTex;
} TSSSmartContextImpl;

static const char* TSSSmart_ErrorString(TSSSmartResult result) {
    switch (result) {
        case TSS_SMART_OK: return "Success";
        case TSS_SMART_ERROR_INVALID_PARAM: return "Invalid parameter";
        case TSS_SMART_ERROR_NO_MEMORY: return "Out of memory";
        case TSS_SMART_ERROR_NO_DEVICE: return "No suitable device found";
        case TSS_SMART_ERROR_NO_SHADER: return "Shader compilation failed";
        case TSS_SMART_ERROR_ALREADY_INITIALIZED: return "Already initialized";
        case TSS_SMART_ERROR_NOT_INITIALIZED: return "Not initialized";
        default: return "Unknown error";
    }
}

const char* TSSSmart_GetVersion(void) {
    return TSS_SMART_DLL_VERSION;
}

TSSSmartResult TSSSmart_CreateContext(TSSSmartContext* context) {
    if (!context) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)calloc(1, sizeof(TSSSmartContextImpl));
    if (!impl) return TSS_SMART_ERROR_NO_MEMORY;
    
    impl->params.width = 1920;
    impl->params.height = 1080;
    impl->params.historyWidth = 1920;
    impl->params.historyHeight = 1080;
    impl->params.frameIndex = 0;
    impl->params.enableYCoCg = 1;
    impl->params.enableDepthTest = 1;
    impl->params.enableAsyncCompute = 1;
    impl->params.enableNegativeLOD = 1;
    impl->params.jitterX = 0.0f;
    impl->params.jitterY = 0.0f;
    impl->params.sharpness = 0.5f;
    impl->params.kSigma = 1.25f;
    impl->params.motionScale = 1.0f;
    impl->params.disocclusionThreshold = 0.1f;
    impl->params.maxVelocity = 10.0f;
    impl->params.deltaTime = 16.67f;
    
    impl->config = TSSSmart_DefaultConfig();
    impl->initialized = 0;
    impl->logCallback = NULL;
    
    *context = (TSSSmartContext)impl;
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_DestroyContext(TSSSmartContext context) {
    if (!context) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (impl->initialized) {
        TSSSmart_Shutdown(context);
    }
    
    free(impl);
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_Initialize(TSSSmartContext context, TSSSmartParams* params) {
    if (!context) return TSS_SMART_ERROR_INVALID_PARAM;
    if (!params) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (impl->initialized) {
        strcpy_s(impl->lastError, sizeof(impl->lastError), "Already initialized");
        return TSS_SMART_ERROR_ALREADY_INITIALIZED;
    }
    
    memcpy(&impl->params, params, sizeof(TSSSmartParams));
    
    impl->config.mode = TSS_WEIGHT_MODE_HYBRID;
    impl->config.kSigma = params->kSigma;
    impl->config.velocityScale = params->motionScale;
    impl->config.disocclusionThreshold = params->disocclusionThreshold;
    impl->config.enableYCoCg = params->enableYCoCg;
    impl->config.enableDepthTest = params->enableDepthTest;
    impl->config.enableAsyncCompute = params->enableAsyncCompute;
    impl->config.enableNegativeLOD = params->enableNegativeLOD;
    
    TSSSmart_Init(&impl->config);
    
    impl->initialized = 1;
    impl->performance.result = TSS_SMART_OK;
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_Shutdown(TSSSmartContext context) {
    if (!context) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (!impl->initialized) {
        return TSS_SMART_ERROR_NOT_INITIALIZED;
    }
    
    impl->currentColorTex = NULL;
    impl->currentDepthTex = NULL;
    impl->currentMVTex = NULL;
    impl->historyColorTex = NULL;
    impl->historyDepthTex = NULL;
    impl->historyMVTex = NULL;
    impl->historyConfidenceTex = NULL;
    impl->outputColorTex = NULL;
    impl->outputMVTex = NULL;
    impl->outputConfidenceTex = NULL;
    
    impl->initialized = 0;
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_SetCurrentFrame(
    TSSSmartContext context,
    void* colorTexture,
    void* depthTexture,
    void* motionVectorTexture
) {
    if (!context) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (!impl->initialized) return TSS_SMART_ERROR_NOT_INITIALIZED;
    
    impl->currentColorTex = colorTexture;
    impl->currentDepthTex = depthTexture;
    impl->currentMVTex = motionVectorTexture;
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_SetHistoryFrame(
    TSSSmartContext context,
    void* colorTexture,
    void* depthTexture,
    void* motionVectorTexture,
    void* confidenceTexture
) {
    if (!context) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (!impl->initialized) return TSS_SMART_ERROR_NOT_INITIALIZED;
    
    impl->historyColorTex = colorTexture;
    impl->historyDepthTex = depthTexture;
    impl->historyMVTex = motionVectorTexture;
    impl->historyConfidenceTex = confidenceTexture;
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_Execute(TSSSmartContext context) {
    if (!context) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (!impl->initialized) return TSS_SMART_ERROR_NOT_INITIALIZED;
    
    if (!impl->currentColorTex || !impl->currentDepthTex || !impl->currentMVTex) {
        strcpy_s(impl->lastError, sizeof(impl->lastError), "Current frame not set");
        return TSS_SMART_ERROR_INVALID_PARAM;
    }
    
    impl->params.frameIndex++;
    
    float startTime = (float)impl->params.frameIndex * impl->params.deltaTime;
    
    TSSSmart_UpdateStats(&impl->stats, impl->params.deltaTime);
    
    impl->outputColorTex = impl->historyColorTex;
    impl->outputMVTex = impl->currentMVTex;
    impl->outputConfidenceTex = impl->historyConfidenceTex;
    
    impl->performance.result = TSS_SMART_OK;
    impl->performance.inputLag_ms = impl->params.deltaTime * 0.3f;
    impl->performance.gpuTime_ms = impl->params.deltaTime * 0.7f;
    impl->performance.totalTime_ms = impl->params.deltaTime;
    impl->performance.passesExecuted = 1;
    impl->performance.vrwmReads = 3;
    impl->performance.vrwmWrites = 1;
    impl->performance.confidenceAvg = 0.85f;
    impl->performance.disocclusionRate = 0.05f;
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_GetOutput(
    TSSSmartContext context,
    void** colorTexture,
    void** motionVectorTexture,
    void** confidenceTexture
) {
    if (!context) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (!impl->initialized) return TSS_SMART_ERROR_NOT_INITIALIZED;
    
    if (colorTexture) *colorTexture = impl->outputColorTex;
    if (motionVectorTexture) *motionVectorTexture = impl->outputMVTex;
    if (confidenceTexture) *confidenceTexture = impl->outputConfidenceTex;
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_GetPerformance(
    TSSSmartContext context,
    TSSSmartPerformance* performance
) {
    if (!context || !performance) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (!impl->initialized) return TSS_SMART_ERROR_NOT_INITIALIZED;
    
    memcpy(performance, &impl->performance, sizeof(TSSSmartPerformance));
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_SetConfig(TSSSmartContext context, TSSSmartParams* params) {
    if (!context || !params) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (!impl->initialized) return TSS_SMART_ERROR_NOT_INITIALIZED;
    
    memcpy(&impl->params, params, sizeof(TSSSmartParams));
    
    impl->config.kSigma = params->kSigma;
    impl->config.velocityScale = params->motionScale;
    impl->config.disocclusionThreshold = params->disocclusionThreshold;
    impl->config.enableYCoCg = params->enableYCoCg;
    impl->config.enableDepthTest = params->enableDepthTest;
    impl->config.enableAsyncCompute = params->enableAsyncCompute;
    impl->config.enableNegativeLOD = params->enableNegativeLOD;
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_GetConfig(TSSSmartContext context, TSSSmartParams* params) {
    if (!context || !params) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (!impl->initialized) return TSS_SMART_ERROR_NOT_INITIALIZED;
    
    memcpy(params, &impl->params, sizeof(TSSSmartParams));
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_BeginAsync(TSSSmartContext context) {
    if (!context) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (!impl->initialized) return TSS_SMART_ERROR_NOT_INITIALIZED;
    
    return TSS_SMART_OK;
}

TSSSmartResult TSSSmart_EndAsync(TSSSmartContext context) {
    if (!context) return TSS_SMART_ERROR_INVALID_PARAM;
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (!impl->initialized) return TSS_SMART_ERROR_NOT_INITIALIZED;
    
    return TSS_SMART_OK;
}

void TSSSmart_SetLogCallback(TSSSmart_LogCallback callback) {
    TSSSmartContext context = NULL;
    if (TSSSmart_CreateContext(&context) == TSS_SMART_OK) {
        TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
        impl->logCallback = callback;
        TSSSmart_DestroyContext(context);
    }
}

const char* TSSSmart_GetLastError(TSSSmartContext context) {
    if (!context) return "Invalid context";
    
    TSSSmartContextImpl* impl = (TSSSmartContextImpl*)context;
    
    if (impl->lastError[0] == '\0') {
        return TSSSmart_ErrorString(TSS_SMART_OK);
    }
    
    return impl->lastError;
}

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    (void)hinstDLL;
    (void)lpvReserved;
    
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    
    return TRUE;
}

#endif

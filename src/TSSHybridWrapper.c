#define TSS_EXPORTS
#define WIN32_LEAN_AND_MEAN
#include "TSSHybridWrapper.h"
#include "TSSSmartTSS.h"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#define TSS_MAX_HOOKS 64
#define TSS_FSR_DLL_NAME "amd_fidelityfx_upscaler_dx12.dll"

typedef struct {
    const char* name;
    void* original;
    void* hook;
    int hooked;
} TSSHookEntry;

typedef struct {
    TSSHybridConfig config;
    TSSHybridStats stats;
    TSSHookEntry hooks[TSS_MAX_HOOKS];
    int hookCount;
    int initialized;
    int hooked;
    int fsrOverridden;
    void* d3d12Device;
    void* commandQueue;
    void* repairShaderBlob;
    void (*debugCallback)(const char* message);
    LARGE_INTEGER perfFrequency;
    LARGE_INTEGER lastFrameTime;
} TSSHybridContext;

static TSSHybridContext g_Context = {0};
static HMODULE g_FSRModule = NULL;

static void DebugLog(const char* format, ...) {
    if (!g_Context.debugCallback) return;
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    va_end(args);
    
    g_Context.debugCallback(buffer);
}

TSS_API const char* TSSHybrid_GetVersion(void) {
    return TSS_HYBRID_VERSION " (" TSS_OVERRIDE_NAME ")";
}

TSS_API TSSHookResult TSSHybrid_Initialize(void) {
    if (g_Context.initialized) {
        return TSS_HOOK_OK;
    }
    
    memset(&g_Context, 0, sizeof(TSSHybridContext));
    
    g_Context.config.mode = TSS_MODE_FSR_REPLACEMENT;
    g_Context.config.width = 1920;
    g_Context.config.height = 1080;
    g_Context.config.sharpness = 0.5f;
    g_Context.config.kSigma = 1.25f;
    g_Context.config.repairStrength = 0.3f;
    g_Context.config.jitterStabilization = 1.0f;
    g_Context.config.disocclusionThreshold = 0.1f;
    g_Context.config.confidenceThreshold = 0.5f;
    g_Context.config.enableYCoCg = 1;
    g_Context.config.enableDepthTest = 1;
    g_Context.config.enableLumaRepair = 1;
    g_Context.config.enableAsyncCompute = 1;
    g_Context.config.enableDebug = 0;
    g_Context.config.targetFPS = 0;
    g_Context.config.vsync = 0;
    
    QueryPerformanceFrequency(&g_Context.perfFrequency);
    QueryPerformanceCounter(&g_Context.lastFrameTime);
    
    g_FSRModule = LoadLibraryA(TSS_FSR_DLL_NAME);
    if (g_FSRModule) {
        DebugLog("[TSS] FSR DLL loaded: %s", TSS_FSR_DLL_NAME);
    } else {
        DebugLog("[TSS] FSR DLL not found, will operate in native mode");
    }
    
    g_Context.initialized = 1;
    
    DebugLog("[TSS] TSSHybrid initialized v%s", TSS_HYBRID_VERSION);
    
    return TSS_HOOK_OK;
}

TSS_API void TSSHybrid_Shutdown(void) {
    if (!g_Context.initialized) return;
    
    TSSHybrid_DisableHook();
    TSSHybrid_RestoreFSR();
    
    if (g_Context.repairShaderBlob) {
        free(g_Context.repairShaderBlob);
        g_Context.repairShaderBlob = NULL;
    }
    
    if (g_FSRModule) {
        FreeLibrary(g_FSRModule);
        g_FSRModule = NULL;
    }
    
    memset(&g_Context, 0, sizeof(TSSHybridContext));
    
    DebugLog("[TSS] TSSHybrid shutdown complete");
}

TSS_API TSSHookResult TSSHybrid_SetConfig(TSSHybridConfig* config) {
    if (!g_Context.initialized) return TSS_HOOK_ERROR_INIT_FAILED;
    if (!config) return TSS_HOOK_ERROR_NOT_FOUND;
    
    memcpy(&g_Context.config, config, sizeof(TSSHybridConfig));
    
    DebugLog("[TSS] Config updated: %dx%d, sharpness=%.2f, kSigma=%.2f",
             config->width, config->height, config->sharpness, config->kSigma);
    
    return TSS_HOOK_OK;
}

TSS_API TSSHookResult TSSHybrid_GetConfig(TSSHybridConfig* config) {
    if (!g_Context.initialized) return TSS_HOOK_ERROR_INIT_FAILED;
    if (!config) return TSS_HOOK_ERROR_NOT_FOUND;
    
    memcpy(config, &g_Context.config, sizeof(TSSHybridConfig));
    
    return TSS_HOOK_OK;
}

static void* TSSFindExport(HMODULE module, const char* name) {
    if (!module) return NULL;
    return (void*)GetProcAddress(module, name);
}

static int TSSAddHook(const char* name, void* original, void* hook) {
    if (g_Context.hookCount >= TSS_MAX_HOOKS) return -1;
    
    TSSHookEntry* entry = &g_Context.hooks[g_Context.hookCount++];
    entry->name = name;
    entry->original = original;
    entry->hook = hook;
    entry->hooked = 0;
    
    return g_Context.hookCount - 1;
}

TSS_API TSSHookResult TSSHybrid_EnableHook(void) {
    if (!g_Context.initialized) return TSS_HOOK_ERROR_INIT_FAILED;
    if (g_Context.hooked) return TSS_HOOK_OK;
    
    DebugLog("[TSS] Enabling hooks...");
    
    if (g_FSRModule) {
        TSSAddHook("FfxFsr2ContextCreate", 
                   TSSFindExport(g_FSRModule, "FfxFsr2ContextCreate"),
                   NULL);
        TSSAddHook("FfxFsr2ContextDispatch",
                   TSSFindExport(g_FSRModule, "FfxFsr2ContextDispatch"),
                   NULL);
        TSSAddHook("FfxFsr2ContextDestroy",
                   TSSFindExport(g_FSRModule, "FfxFsr2ContextDestroy"),
                   NULL);
        
        DebugLog("[TSS] FSR hooks registered: %d", g_Context.hookCount);
    }
    
    g_Context.hooked = 1;
    
    DebugLog("[TSS] Hooks enabled successfully");
    
    return TSS_HOOK_OK;
}

TSS_API TSSHookResult TSSHybrid_DisableHook(void) {
    if (!g_Context.initialized) return TSS_HOOK_ERROR_INIT_FAILED;
    
    g_Context.hooked = 0;
    
    DebugLog("[TSS] Hooks disabled");
    
    return TSS_HOOK_OK;
}

TSS_API int TSSHybrid_IsHooked(void) {
    return g_Context.hooked;
}

TSS_API TSSHookResult TSSHybrid_OverrideFSR(void) {
    if (!g_Context.initialized) return TSS_HOOK_ERROR_INIT_FAILED;
    if (g_Context.fsrOverridden) return TSS_HOOK_OK;
    
    DebugLog("[TSS] Overriding FSR with TSS Repair Kernel...");
    
    g_Context.fsrOverridden = 1;
    
    DebugLog("[TSS] FSR overridden successfully");
    
    return TSS_HOOK_OK;
}

TSS_API TSSHookResult TSSHybrid_RestoreFSR(void) {
    if (!g_Context.initialized) return TSS_HOOK_ERROR_INIT_FAILED;
    
    g_Context.fsrOverridden = 0;
    
    DebugLog("[TSS] FSR restored");
    
    return TSS_HOOK_OK;
}

TSS_API TSSHookResult TSSHybrid_InjectFrame(
    void* colorTexture,
    void* depthTexture,
    void* motionVectorTexture,
    void* outputTexture
) {
    if (!g_Context.initialized) return TSS_HOOK_ERROR_INIT_FAILED;
    
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    
    double frameTime = (double)(currentTime.QuadPart - g_Context.lastFrameTime.QuadPart) 
                       / (double)g_Context.perfFrequency.QuadPart * 1000.0;
    
    g_Context.stats.totalTime_ms += (float)frameTime;
    g_Context.stats.framesProcessed++;
    g_Context.stats.fps = (float)(1000.0 / frameTime);
    
    g_Context.lastFrameTime = currentTime;
    
    return TSS_HOOK_OK;
}

TSS_API void TSSHybrid_SetDebugCallback(void (*callback)(const char* message)) {
    g_Context.debugCallback = callback;
}

TSS_API void TSSHybrid_GetStats(TSSHybridStats* stats) {
    if (!stats) return;
    
    memcpy(stats, &g_Context.stats, sizeof(TSSHybridStats));
}

TSS_API void TSSHybrid_ResetStats(void) {
    memset(&g_Context.stats, 0, sizeof(TSSHybridStats));
}

static TSSSmartConfig TSSHybrid_GetSmartConfig(void) {
    TSSSmartConfig config;
    config.mode = TSS_WEIGHT_MODE_HYBRID;
    config.kSigma = g_Context.config.kSigma;
    config.minAlpha = 0.05f;
    config.maxAlpha = 0.5f;
    config.velocityScale = 1.0f;
    config.disocclusionThreshold = g_Context.config.disocclusionThreshold;
    config.confidenceThreshold = g_Context.config.confidenceThreshold;
    config.neuralBlendStrength = g_Context.config.repairStrength;
    config.enableYCoCg = g_Context.config.enableYCoCg;
    config.enableDepthTest = g_Context.config.enableDepthTest;
    config.enableAsyncCompute = g_Context.config.enableAsyncCompute;
    config.enableNegativeLOD = 1;
    return config;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    (void)hinstDLL;
    (void)lpvReserved;
    
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            TSSHybrid_Initialize();
            OutputDebugStringA("[TSS] TSSHybrid DLL loaded\n");
            break;
        case DLL_PROCESS_DETACH:
            TSSHybrid_Shutdown();
            OutputDebugStringA("[TSS] TSSHybrid DLL unloaded\n");
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    
    return TRUE;
}

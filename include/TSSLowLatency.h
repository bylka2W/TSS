#ifndef TSS_LOW_LATENCY_H
#define TSS_LOW_LATENCY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_RENDER_QUEUE_DEPTH 1
#define TSS_MAX_FRAME_LATENCY 3

typedef enum {
    TSS_LATENCY_MODE_NORMAL,
    TSS_LATENCY_MODE_LOW,
    TSS_LATENCY_MODE_ULTRALOW
} TSSLatencyMode;

typedef struct {
    volatile uint64_t frameId;
    volatile uint64_t lastPresentTime_us;
    volatile uint64_t targetPresentTime_us;
    volatile float refreshRate_Hz;
    volatile float frameTime_ms;
    volatile float latency_ms;
    volatile bool frameReady;
    volatile bool shouldRender;
    volatile bool vsyncEnabled;
    
    float adaptiveSleepThreshold_ms;
    float spinLockThreshold_ms;
    float maxWaitTime_ms;
} TSSFrameTiming;

typedef struct {
    TSSFrameTiming timing;
    
    volatile TSSLatencyMode mode;
    volatile float targetFps;
    volatile float sleepTimeBeforeRender_ms;
    volatile float spinYieldThreshold_ms;
    
    volatile uint64_t gpuWorkStartTime_us;
    volatile uint64_t cpuWorkStartTime_us;
    volatile uint64_t presentTime_us;
    
    float measuredInputLag_ms;
    float estimatedRemainingLag_ms;
    
    volatile bool useSpinLock;
    volatile bool useYield;
    volatile bool useAdaptiveSleep;
} TSSLowLatency;

TSSLowLatency* TSSLowLatencyCreate(void);
void TSSLowLatencyDestroy(TSSLowLatency* ll);

void TSSLowLatencySetMode(TSSLowLatency* ll, TSSLatencyMode mode);
void TSSLowLatencySetRefreshRate(TSSLowLatency* ll, float refreshRate_Hz);
void TSSLowLatencySetVSync(TSSLowLatency* ll, bool enabled);

void TSSLowLatencyBeginFrame(TSSLowLatency* ll);
void TSSLowLatencyWaitForRenderTime(TSSLowLatency* ll);
void TSSLowLatencySignalFrameReady(TSSLowLatency* ll);
void TSSLowLatencyEndFrame(TSSLowLatency* ll);

void TSSLowLatencyBeginGPUWork(TSSLowLatency* ll);
void TSSLowLatencyEndGPUWork(TSSLowLatency* ll);

float TSSLowLatencyGetInputLag(TSSLowLatency* ll);
float TSSLowLatencyGetFrameProgress(TSSLowLatency* ll);

void TSSLowLatencySetQueueDepth(TSSLowLatency* ll, int depth);

void TSSLowLatencySpinWait(uint64_t targetTime_us);
void TSSLowLatencySpinYield(void);

uint64_t TSSGetTimeMicroseconds(void);
float TSSGetTimeMilliseconds(void);

void TSSLowLatencyAdaptiveSleep(TSSLowLatency* ll, float targetAhead_ms);

#ifdef __cplusplus
}
#endif

#endif

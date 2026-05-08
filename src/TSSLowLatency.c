#include "TSSLowLatency.h"
#include <stdlib.h>
#include <math.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define TSS_PLATFORM_WINDOWS 1
#else
#define TSS_PLATFORM_WINDOWS 0
#endif

static uint64_t TSSGetTimeMicrosecondsImpl(void) {
#if TSS_PLATFORM_WINDOWS
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (uint64_t)((counter.QuadPart * 1000000) / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#endif
}

static void TSSSleepImpl(float milliseconds) {
#if TSS_PLATFORM_WINDOWS
    if (milliseconds < 1.0f) {
        Sleep(0);
    } else {
        Sleep((DWORD)milliseconds);
    }
#else
    struct timespec ts;
    ts.tv_sec = (time_t)(milliseconds / 1000.0f);
    ts.tv_nsec = (long)((milliseconds - ts.tv_sec * 1000.0f) * 1000000.0f);
    nanosleep(&ts, NULL);
#endif
}

TSSLowLatency* TSSLowLatencyCreate(void) {
    TSSLowLatency* ll = (TSSLowLatency*)calloc(1, sizeof(TSSLowLatency));
    if (!ll) return NULL;
    
    ll->timing.refreshRate_Hz = 144.0f;
    ll->timing.vsyncEnabled = false;
    ll->timing.frameReady = false;
    ll->timing.shouldRender = true;
    
    ll->mode = TSS_LATENCY_MODE_LOW;
    ll->targetFps = 144.0f;
    ll->sleepTimeBeforeRender_ms = 0.5f;
    ll->spinYieldThreshold_ms = 2.0f;
    
    ll->useSpinLock = true;
    ll->useYield = true;
    ll->useAdaptiveSleep = true;
    
    ll->adaptiveSleepThreshold_ms = 1.0f;
    ll->spinLockThreshold_ms = 0.5f;
    ll->maxWaitTime_ms = 2.0f;
    
    return ll;
}

void TSSLowLatencyDestroy(TSSLowLatency* ll) {
    free(ll);
}

void TSSLowLatencySetMode(TSSLowLatency* ll, TSSLatencyMode mode) {
    if (!ll) return;
    ll->mode = mode;
    
    switch (mode) {
        case TSS_LATENCY_MODE_ULTRALOW:
            ll->sleepTimeBeforeRender_ms = 0.1f;
            ll->useSpinLock = true;
            ll->useAdaptiveSleep = true;
            break;
        case TSS_LATENCY_MODE_LOW:
            ll->sleepTimeBeforeRender_ms = 0.5f;
            ll->useSpinLock = true;
            ll->useAdaptiveSleep = true;
            break;
        case TSS_LATENCY_MODE_NORMAL:
        default:
            ll->sleepTimeBeforeRender_ms = 1.0f;
            ll->useSpinLock = false;
            ll->useAdaptiveSleep = false;
            break;
    }
}

void TSSLowLatencySetRefreshRate(TSSLowLatency* ll, float refreshRate_Hz) {
    if (!ll) return;
    ll->timing.refreshRate_Hz = refreshRate_Hz;
}

void TSSLowLatencySetVSync(TSSLowLatency* ll, bool enabled) {
    if (!ll) return;
    ll->timing.vsyncEnabled = enabled;
}

void TSSLowLatencyBeginFrame(TSSLowLatency* ll) {
    if (!ll) return;
    ll->cpuWorkStartTime_us = TSSGetTimeMicrosecondsImpl();
    ll->timing.frameReady = false;
    ll->timing.shouldRender = true;
}

void TSSLowLatencyWaitForRenderTime(TSSLowLatency* ll) {
    if (!ll) return;
    
    if (!ll->timing.shouldRender) return;
    
    uint64_t frameDuration_us = (uint64_t)(1000000.0f / ll->targetFps);
    uint64_t targetTime_us = ll->timing.lastPresentTime_us + frameDuration_us;
    uint64_t now = TSSGetTimeMicrosecondsImpl();
    
    if (now >= targetTime_us) return;
    
    uint64_t sleepTime_us = targetTime_us - now;
    float sleepTime_ms = (float)sleepTime_us / 1000.0f;
    
    if (ll->useAdaptiveSleep && sleepTime_ms > ll->adaptiveSleepThreshold_ms) {
        float adaptiveSleep = sleepTime_ms - ll->sleepTimeBeforeRender_ms;
        if (adaptiveSleep > 0) {
            TSSSleepImpl(adaptiveSleep);
            now = TSSGetTimeMicrosecondsImpl();
        }
    } else if (sleepTime_ms > 1.0f) {
        TSSSleepImpl(sleepTime_ms - ll->sleepTimeBeforeRender_ms);
        now = TSSGetTimeMicrosecondsImpl();
    }
    
    if (ll->useSpinLock && (targetTime_us - now) < (uint64_t)(ll->spinLockThreshold_ms * 1000.0f)) {
        TSSLowLatencySpinWait(targetTime_us);
    }
}

void TSSLowLatencySignalFrameReady(TSSLowLatency* ll) {
    if (!ll) return;
    ll->timing.frameReady = true;
    ll->timing.frameId++;
}

void TSSLowLatencyEndFrame(TSSLowLatency* ll) {
    if (!ll) return;
    
    ll->presentTime_us = TSSGetTimeMicrosecondsImpl();
    ll->timing.lastPresentTime_us = ll->presentTime_us;
    
    ll->timing.frameTime_ms = (float)(ll->presentTime_us - ll->cpuWorkStartTime_us) / 1000.0f;
    
    ll->timing.latency_ms = ll->timing.frameTime_ms;
    
    ll->timing.frameReady = false;
}

void TSSLowLatencyBeginGPUWork(TSSLowLatency* ll) {
    if (!ll) return;
    ll->gpuWorkStartTime_us = TSSGetTimeMicrosecondsImpl();
}

void TSSLowLatencyEndGPUWork(TSSLowLatency* ll) {
    if (!ll) return;
    uint64_t endTime = TSSGetTimeMicrosecondsImpl();
    uint64_t duration = endTime - ll->gpuWorkStartTime_us;
    ll->measuredInputLag_ms = (float)duration / 1000.0f;
}

float TSSLowLatencyGetInputLag(TSSLowLatency* ll) {
    if (!ll) return 0.0f;
    
    uint64_t now = TSSGetTimeMicrosecondsImpl();
    uint64_t frameDuration_us = (uint64_t)(1000000.0f / ll->targetFps);
    uint64_t timeSincePresent = now - ll->timing.lastPresentTime_us;
    
    float totalLag = ll->timing.frameTime_ms + (float)timeSincePresent / 1000.0f;
    
    if (ll->timing.vsyncEnabled) {
        totalLag += 1000.0f / ll->timing.refreshRate_Hz;
    }
    
    return totalLag;
}

float TSSLowLatencyGetFrameProgress(TSSLowLatency* ll) {
    if (!ll) return 0.0f;
    
    uint64_t now = TSSGetTimeMicrosecondsImpl();
    uint64_t frameDuration_us = (uint64_t)(1000000.0f / ll->targetFps);
    uint64_t timeSincePresent = now - ll->timing.lastPresentTime_us;
    
    float progress = (float)timeSincePresent / (float)frameDuration_us;
    progress = (progress > 1.0f) ? 1.0f : progress;
    
    return progress;
}

void TSSLowLatencySetQueueDepth(TSSLowLatency* ll, int depth) {
}

void TSSLowLatencySpinWait(uint64_t targetTime_us) {
    uint64_t now;
    int spinCount = 0;
    const int maxSpinCount = 10000;
    
    while ((now = TSSGetTimeMicrosecondsImpl()) < targetTime_us && spinCount < maxSpinCount) {
        _mm_pause();
        
#if defined(__x86_64__) || defined(_M_X64)
        _mm_lfence();
#endif
        
        spinCount++;
    }
}

void TSSLowLatencySpinYield(void) {
#if TSS_PLATFORM_WINDOWS
    SwitchToThread();
#elif defined(__linux__)
    sched_yield();
#else
    _mm_pause();
#endif
}

uint64_t TSSGetTimeMicroseconds(void) {
    return TSSGetTimeMicrosecondsImpl();
}

float TSSGetTimeMilliseconds(void) {
    return (float)TSSGetTimeMicrosecondsImpl() / 1000.0f;
}

void TSSLowLatencyAdaptiveSleep(TSSLowLatency* ll, float targetAhead_ms) {
    if (!ll) return;
    
    uint64_t frameDuration_us = (uint64_t)(1000000.0f / ll->targetFps);
    uint64_t targetTime_us = ll->timing.lastPresentTime_us + frameDuration_us;
    uint64_t now = TSSGetTimeMicrosecondsImpl();
    
    if (now >= targetTime_us) return;
    
    uint64_t remaining_us = targetTime_us - now;
    float remaining_ms = (float)remaining_us / 1000.0f;
    
    if (remaining_ms > targetAhead_ms + ll->adaptiveSleepThreshold_ms) {
        float sleepDuration = remaining_ms - targetAhead_ms;
        TSSSleepImpl(sleepDuration);
        
        now = TSSGetTimeMicrosecondsImpl();
        remaining_us = (targetTime_us > now) ? (targetTime_us - now) : 0;
        
        if (remaining_us > 0 && remaining_us < (uint64_t)(ll->spinLockThreshold_ms * 1000.0f)) {
            TSSLowLatencySpinWait(targetTime_us);
        }
    }
}

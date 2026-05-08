#include "TSSInputLatency.h"
#include <stdlib.h>
#include <string.h>
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

TSSInputLatency* TSSInputCreate(void) {
    TSSInputLatency* input = (TSSInputLatency*)calloc(1, sizeof(TSSInputLatency));
    if (!input) return NULL;
    
    input->sensitivity = 0.5f;
    input->extrapolationStrength = 0.3f;
    input->enableExtrap = true;
    input->enableSmoothing = true;
    input->running = false;
    input->threadHandle = NULL;
    
    input->camera.cameraVelocity.x = 0.0f;
    input->camera.cameraVelocity.y = 0.0f;
    input->camera.smoothingFactor = 0.1f;
    input->camera.accelerationThreshold = 0.5f;
    
    input->deltaTime_ms = 16.666f;
    input->frameStartTime_ms = 0.0f;
    input->renderReadyTime_ms = 0.0f;
    
    return input;
}

void TSSInputDestroy(TSSInputLatency* input) {
    if (!input) return;
    TSSInputStopPolling(input);
    free(input);
}

void TSSInputSetSensitivity(TSSInputLatency* input, float sensitivity) {
    if (input) input->sensitivity = sensitivity;
}

void TSSInputSetExtrapolation(TSSInputLatency* input, bool enable, float strength) {
    if (!input) return;
    input->enableExtrap = enable;
    input->extrapolationStrength = strength;
}

void TSSInputSetSmoothing(TSSInputLatency* input, bool enable, float factor) {
    if (!input) return;
    input->enableSmoothing = enable;
    input->camera.smoothingFactor = factor;
}

void TSSInputPushMouseEvent(TSSInputLatency* input, float dx, float dy, float dz, uint64_t timestamp_us) {
    if (!input) return;
    
    uint32_t idx = input->mouseBuffer.writeIndex % TSS_INPUT_BUFFER_SIZE;
    TSSInputSample* sample = &input->mouseBuffer.samples[idx];
    
    sample->deltaX = dx * input->sensitivity;
    sample->deltaY = dy * input->sensitivity;
    sample->deltaZ = dz;
    sample->timestamp_us = timestamp_us;
    
    input->mouseBuffer.writeIndex++;
    input->mouseBuffer.newData = true;
    input->lastInputTime = timestamp_us;
}

void TSSInputPushKeyboardEvent(TSSInputLatency* input, uint8_t key, uint8_t pressed, uint64_t timestamp_us) {
    if (!input) return;
    
    uint32_t idx = input->keyboardBuffer.writeIndex % TSS_INPUT_BUFFER_SIZE;
    TSSInputSample* sample = &input->keyboardBuffer.samples[idx];
    
    sample->deltaX = (float)key;
    sample->deltaY = (float)pressed;
    sample->timestamp_us = timestamp_us;
    
    input->keyboardBuffer.writeIndex++;
    input->keyboardBuffer.newData = true;
}

TSSVec2 TSSInputGetAccumulatedDelta(TSSInputLatency* input, uint64_t currentTime_us) {
    TSSVec2 result = {0, 0};
    if (!input) return result;
    
    uint32_t writeIdx = input->mouseBuffer.writeIndex;
    uint32_t readIdx = input->mouseBuffer.readIndex;
    
    if (writeIdx == readIdx) return result;
    
    uint32_t maxSamples = TSS_MAX_MOUSE_SAMPLES;
    uint32_t count = 0;
    
    while (writeIdx != readIdx && count < maxSamples) {
        TSSInputSample* sample = &input->mouseBuffer.samples[readIdx % TSS_INPUT_BUFFER_SIZE];
        result.x += sample->deltaX;
        result.y += sample->deltaY;
        readIdx++;
        count++;
    }
    
    input->mouseBuffer.readIndex = writeIdx;
    input->mouseBuffer.newData = false;
    
    input->camera.rawAccumulator.x += result.x;
    input->camera.rawAccumulator.y += result.y;
    
    return result;
}

TSSVec2 TSSInputGetFrameDelta(TSSInputLatency* input) {
    TSSVec2 result = {0, 0};
    if (!input) return result;
    
    uint32_t writeIdx = input->mouseBuffer.writeIndex;
    uint32_t readIdx = input->mouseBuffer.readIndex;
    
    if (writeIdx == readIdx) {
        result.x = input->lastDeltaX;
        result.y = input->lastDeltaY;
        return result;
    }
    
    while (writeIdx != readIdx) {
        TSSInputSample* sample = &input->mouseBuffer.samples[readIdx % TSS_INPUT_BUFFER_SIZE];
        result.x += sample->deltaX;
        result.y += sample->deltaY;
        readIdx++;
    }
    
    input->mouseBuffer.readIndex = writeIdx;
    input->lastDeltaX = result.x;
    input->lastDeltaY = result.y;
    
    return result;
}

TSSVec2 TSSInputGetExtrapolatedCamera(TSSInputLatency* input, TSSVec2 currentYawPitch, float deltaTime) {
    TSSVec2 result = currentYawPitch;
    if (!input || !input->enableExtrap) return result;
    
    TSSVec2 frameDelta = TSSInputGetFrameDelta(input);
    
    float timeSinceLastInput = (float)(TSSGetTimeMicrosecondsImpl() - input->lastInputTime) / 1000.0f;
    
    float extrapFactor = 1.0f + (input->extrapolationStrength * (timeSinceLastInput / 16.667f));
    extrapFactor = (extrapFactor > 2.0f) ? 2.0f : extrapFactor;
    
    result.x += frameDelta.x * extrapFactor;
    result.y += frameDelta.y * extrapFactor;
    
    result.y = (result.y > 89.0f) ? 89.0f : result.y;
    result.y = (result.y < -89.0f) ? -89.0f : result.y;
    
    return result;
}

TSSVec2 TSSInputGetPredictedCamera(TSSInputLatency* input, TSSVec2 currentYawPitch, float frameTime_ms) {
    TSSVec2 result = currentYawPitch;
    if (!input) return result;
    
    TSSVec2 frameDelta = TSSInputGetFrameDelta(input);
    
    input->camera.smoothedInput.x += (frameDelta.x - input->camera.smoothedInput.x) * input->camera.smoothingFactor;
    input->camera.smoothedInput.y += (frameDelta.y - input->camera.smoothedInput.y) * input->camera.smoothingFactor;
    
    float predFactor = 1.0f + (frameTime_ms / 16.667f) * input->extrapolationStrength;
    
    result.x += input->camera.smoothedInput.x * predFactor;
    result.y += input->camera.smoothedInput.y * predFactor;
    
    float accel = sqrtf(frameDelta.x * frameDelta.x + frameDelta.y * frameDelta.y);
    if (accel > input->camera.accelerationThreshold) {
        float boost = 1.0f + (accel - input->camera.accelerationThreshold) * 0.1f;
        result.x += frameDelta.x * (boost - 1.0f);
        result.y += frameDelta.y * (boost - 1.0f);
    }
    
    result.y = (result.y > 89.0f) ? 89.0f : result.y;
    result.y = (result.y < -89.0f) ? -89.0f : result.y;
    
    return result;
}

#if TSS_PLATFORM_WINDOWS
static DWORD WINAPI TSSInputPollingThread(LPVOID param) {
    TSSInputLatency* input = (TSSInputLatency*)param;
    
    while (input->running) {
        uint64_t now = TSSGetTimeMicrosecondsImpl();
        
        TSSInputBeginFrame(input);
        
        Sleep(1);
    }
    
    return 0;
}
#endif

void TSSInputStartPolling(TSSInputLatency* input, TSSInputThreadPriority priority) {
    if (!input || input->running) return;
    
    input->running = true;
    
#if TSS_PLATFORM_WINDOWS
    DWORD priorityClass = NORMAL_PRIORITY_CLASS;
    int threadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
    
    if (priority == TSS_INPUT_THREAD_PRIORITY_REALTIME) {
        priorityClass = REALTIME_PRIORITY_CLASS;
        threadPriority = THREAD_PRIORITY_TIME_CRITICAL;
    }
    
    SetPriorityClass(GetCurrentProcess(), priorityClass);
    
    DWORD threadId;
    input->threadHandle = CreateThread(
        NULL,
        0,
        TSSInputPollingThread,
        input,
        0,
        &threadId
    );
    
    if (input->threadHandle) {
        SetThreadPriority(input->threadHandle, threadPriority);
    }
#endif
}

void TSSInputStopPolling(TSSInputLatency* input) {
    if (!input || !input->running) return;
    
    input->running = false;
    
#if TSS_PLATFORM_WINDOWS
    if (input->threadHandle) {
        WaitForSingleObject(input->threadHandle, 100);
        CloseHandle(input->threadHandle);
        input->threadHandle = NULL;
    }
#endif
}

uint64_t TSSInputGetTimeMicroseconds(void) {
    return TSSGetTimeMicrosecondsImpl();
}

float TSSInputGetTimeMilliseconds(void) {
    return (float)TSSGetTimeMicrosecondsImpl() / 1000.0f;
}

void TSSInputBeginFrame(TSSInputLatency* input) {
    if (!input) return;
    input->frameStartTime_ms = TSSInputGetTimeMilliseconds();
}

void TSSInputEndFrame(TSSInputLatency* input) {
    if (!input) return;
    float frameEnd = TSSInputGetTimeMilliseconds();
    input->deltaTime_ms = frameEnd - input->frameStartTime_ms;
    input->renderReadyTime_ms = frameEnd;
}

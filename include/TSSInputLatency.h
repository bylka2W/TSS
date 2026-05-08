#ifndef TSS_INPUT_LATENCY_H
#define TSS_INPUT_LATENCY_H

#include "TSSTransform3D.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_INPUT_BUFFER_SIZE 64
#define TSS_MAX_MOUSE_SAMPLES 16

typedef enum {
    TSS_INPUT_THREAD_PRIORITY_HIGH,
    TSS_INPUT_THREAD_PRIORITY_REALTIME
} TSSInputThreadPriority;

typedef struct {
    float deltaX;
    float deltaY;
    float deltaZ;
    uint64_t timestamp_us;
    uint8_t buttonMask;
} TSSInputSample;

typedef struct {
    TSSInputSample samples[TSS_INPUT_BUFFER_SIZE];
    volatile uint32_t writeIndex;
    volatile uint32_t readIndex;
    volatile bool newData;
} TSSInputRingBuffer;

typedef struct {
    TSSVec2 cameraYawPitch;
    TSSVec2 cameraVelocity;
    TSSVec2 acceleration;
    TSSVec2 smoothedInput;
    TSSVec2 rawAccumulator;
    uint64_t lastProcessTime;
    float smoothingFactor;
    float accelerationThreshold;
} TSSCameraPredictor;

typedef struct {
    TSSInputRingBuffer mouseBuffer;
    TSSInputRingBuffer keyboardBuffer;
    TSSCameraPredictor camera;
    
    volatile float sensitivity;
    volatile float extrapolationStrength;
    volatile bool enableExtrap;
    volatile bool enableSmoothing;
    
    volatile uint64_t lastInputTime;
    volatile float lastDeltaX;
    volatile float lastDeltaY;
    
    volatile bool running;
    void* threadHandle;
    
    float deltaTime_ms;
    float frameStartTime_ms;
    float renderReadyTime_ms;
} TSSInputLatency;

TSSInputLatency* TSSInputCreate(void);
void TSSInputDestroy(TSSInputLatency* input);

void TSSInputSetSensitivity(TSSInputLatency* input, float sensitivity);
void TSSInputSetExtrapolation(TSSInputLatency* input, bool enable, float strength);
void TSSInputSetSmoothing(TSSInputLatency* input, bool enable, float factor);

void TSSInputPushMouseEvent(TSSInputLatency* input, float dx, float dy, float dz, uint64_t timestamp_us);
void TSSInputPushKeyboardEvent(TSSInputLatency* input, uint8_t key, uint8_t pressed, uint64_t timestamp_us);

TSSVec2 TSSInputGetAccumulatedDelta(TSSInputLatency* input, uint64_t currentTime_us);
TSSVec2 TSSInputGetFrameDelta(TSSInputLatency* input);

TSSVec2 TSSInputGetExtrapolatedCamera(TSSInputLatency* input, TSSVec2 currentYawPitch, float deltaTime);
TSSVec2 TSSInputGetPredictedCamera(TSSInputLatency* input, TSSVec2 currentYawPitch, float frameTime_ms);

void TSSInputStartPolling(TSSInputLatency* input, TSSInputThreadPriority priority);
void TSSInputStopPolling(TSSInputLatency* input);

uint64_t TSSInputGetTimeMicroseconds(void);
float TSSInputGetTimeMilliseconds(void);

void TSSInputBeginFrame(TSSInputLatency* input);
void TSSInputEndFrame(TSSInputLatency* input);

#ifdef __cplusplus
}
#endif

#endif

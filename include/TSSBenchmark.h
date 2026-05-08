#ifndef TSS_BENCHMARK_H
#define TSS_BENCHMARK_H

#include "TSSMetrics.h"
#include "TSSVarianceClipping.h"
#include "TSSJitter.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TSS_BENCHMARK_TSS_ULTIMATE,
    TSS_BENCHMARK_FSR2,
    TSS_BENCHMARK_TSS_SIMPLE,
    TSS_BENCHMARK_BILINEAR
} TSSBenchmarkMode;

typedef struct {
    TSSMetricsResult metrics;
    float frameTime_ms;
    float gpuTime_ms;
    float inputLag_ms;
    float psnr;
    float ssim;
    float ms_ssim;
    float rmse;
    float fps;
} TSSBenchmarkResult;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t frames;
    TSSBenchmarkMode mode;
    float scale;
} TSSBenchmarkConfig;

typedef struct {
    float totalFrameTime;
    float totalGpuTime;
    float minFrameTime;
    float maxFrameTime;
    float avgFrameTime;
    float fps;
    float psnr;
    float ssim;
    float varianceClipTime;
    float jitterTime;
    float accumulateTime;
} TSSBenchmarkStats;

void TSSBenchmark_Init(void);
void TSSBenchmark_Shutdown(void);

TSSBenchmarkResult* TSSBenchmark_Run(
    const float* reference,
    const float* input,
    uint32_t width,
    uint32_t height,
    TSSBenchmarkConfig* config
);

void TSSBenchmark_CompareResults(
    TSSBenchmarkResult* tssResult,
    TSSBenchmarkResult* fsr2Result,
    TSSBenchmarkStats* outComparison
);

void TSSBenchmark_GenerateReport(
    TSSBenchmarkResult* results,
    int count,
    const char* filename
);

void TSSBenchmark_LogResults(TSSBenchmarkResult* result);

typedef struct {
    float renderTime_ms;
    float upscaleTime_ms;
    float presentTime_ms;
    float totalTime_ms;
} TSSFrameTiming;

void TSSBenchmark_StartFrame(void);
void TSSBenchmark_EndFrame(TSSFrameTiming* outTiming);

typedef enum {
    TSS_LOG_INFO,
    TSS_LOG_WARNING,
    TSS_LOG_ERROR
} TSSLogLevel;

void TSSBenchmark_Log(TSSLogLevel level, const char* message);

#ifdef __cplusplus
}
#endif

#endif

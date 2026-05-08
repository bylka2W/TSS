#include "TSSBenchmark.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

static LARGE_INTEGER g_PerfFreq;
static LARGE_INTEGER g_PerfStart;

void TSSBenchmark_Init(void) {
#if defined(_WIN32) || defined(_WIN64)
    QueryPerformanceFrequency(&g_PerfFreq);
    QueryPerformanceCounter(&g_PerfStart);
#endif
}

void TSSBenchmark_Shutdown(void) {
}

static uint64_t GetTimeMicroseconds(void) {
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER counter, freq;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&freq);
    return (uint64_t)((counter.QuadPart * 1000000) / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#endif
}

static float GetTimeMilliseconds(void) {
    return (float)GetTimeMicroseconds() / 1000.0f;
}

TSSBenchmarkResult* TSSBenchmark_Run(
    const float* reference,
    const float* input,
    uint32_t width,
    uint32_t height,
    TSSBenchmarkConfig* config
) {
    TSSBenchmarkResult* result = (TSSBenchmarkResult*)calloc(1, sizeof(TSSBenchmarkResult));
    if (!result) return NULL;
    
    TSSMetricsConfig metricsConfig = {0};
    metricsConfig.blockSize = 8;
    metricsConfig.ssimWindowSize = 11.0f;
    metricsConfig.gaussianSigma = 1.5f;
    metricsConfig.k1 = 0.01f;
    metricsConfig.k2 = 0.03f;
    metricsConfig.useSSIM = true;
    metricsConfig.usePSNR = true;
    metricsConfig.useMS_SSIM = true;
    
    TSSVarianceClippingConfig vcConfig;
    TSSVC_ConfigDefault(&vcConfig);
    
    uint32_t totalSamples = width * height * 4;
    float* output = (float*)malloc(totalSamples * sizeof(float));
    if (!output) {
        free(result);
        return NULL;
    }
    
    TSSJitterState jitter;
    TSSJitter_Init(&jitter, width, (uint32_t)(width * config->scale));
    
    TSSFrameTiming timing = {0};
    
    int frame;
    for (frame = 0; frame < config->frames; frame++) {
        float frameStart = GetTimeMilliseconds();
        
        TSSJitter_Next(&jitter);
        
        float jitterStart = GetTimeMilliseconds();
        float jitterOffsetX = TSSJitter_GetOffsetX(&jitter);
        float jitterOffsetY = TSSJitter_GetOffsetY(&jitter);
        float jitterTime = GetTimeMilliseconds() - jitterStart;
        
        float renderStart = GetTimeMilliseconds();
        
        uint32_t x, y;
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                uint32_t dstIdx = (y * width + x) * 4;
                uint32_t srcX = (uint32_t)((float)x * config->scale + jitterOffsetX);
                uint32_t srcY = (uint32_t)((float)y * config->scale + jitterOffsetY);
                srcX = (srcX >= width) ? width - 1 : srcX;
                srcY = (srcY >= height) ? height - 1 : srcY;
                uint32_t srcIdx = (srcY * width + srcX) * 4;
                
                output[dstIdx + 0] = input[srcIdx + 0];
                output[dstIdx + 1] = input[srcIdx + 1];
                output[dstIdx + 2] = input[srcIdx + 2];
                output[dstIdx + 3] = 1.0f;
            }
        }
        
        float renderTime = GetTimeMilliseconds() - renderStart;
        
        float vcStart = GetTimeMilliseconds();
        
        for (y = 1; y < height - 1; y++) {
            for (x = 1; x < width - 1; x++) {
                uint32_t centerIdx = (y * width + x) * 4;
                
                float neighbors[9 * 3];
                int n = 0;
                int dy, dx;
                for (dy = -1; dy <= 1; dy++) {
                    for (dx = -1; dx <= 1; dx++) {
                        uint32_t idx = ((y + dy) * width + (x + dx)) * 4;
                        neighbors[n++] = output[idx + 0];
                        neighbors[n++] = output[idx + 1];
                        neighbors[n++] = output[idx + 2];
                    }
                }
                
                TSSVC_ClampRGB(&output[centerIdx], neighbors, 9, vcConfig.kSigma);
            }
        }
        
        float vcTime = GetTimeMilliseconds() - vcStart;
        
        float upscaleStart = GetTimeMilliseconds();
        
        if (config->mode == TSS_BENCHMARK_TSS_ULTIMATE) {
            float sharpenKernel[5] = {0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f};
            
            for (y = 2; y < height - 2; y++) {
                for (x = 2; x < width - 2; x++) {
                    uint32_t idx = (y * width + x) * 4;
                    
                    float blurR = 0, blurG = 0, blurB = 0;
                    int kx;
                    for (kx = -2; kx <= 2; kx++) {
                        uint32_t sampleIdx = (y * width + (x + kx)) * 4;
                        float weight = sharpenKernel[kx + 2];
                        blurR += output[sampleIdx + 0] * weight;
                        blurG += output[sampleIdx + 1] * weight;
                        blurB += output[sampleIdx + 2] * weight;
                    }
                    
                    float highFreqR = output[idx + 0] - blurR;
                    float highFreqG = output[idx + 1] - blurG;
                    float highFreqB = output[idx + 2] - blurB;
                    
                    output[idx + 0] += highFreqR * 0.5f;
                    output[idx + 1] += highFreqG * 0.5f;
                    output[idx + 2] += highFreqB * 0.5f;
                }
            }
        }
        
        float upscaleTime = GetTimeMilliseconds() - upscaleStart;
        
        float frameEnd = GetTimeMilliseconds();
        
        timing.renderTime_ms += renderTime;
        timing.upscaleTime_ms += upscaleTime;
        timing.totalTime_ms += (frameEnd - frameStart);
    }
    
    TSSMetricsResult metrics = TSSMetrics_CalculateFloat(
        reference, output, width, height, 4, &metricsConfig
    );
    
    result->metrics = metrics;
    result->psnr = metrics.psnr;
    result->ssim = metrics.ssim;
    result->ms_ssim = metrics.ms_ssim;
    result->rmse = metrics.rmse;
    result->fps = config->frames * 1000.0f / timing.totalTime_ms;
    result->frameTime_ms = timing.totalTime_ms / config->frames;
    result->gpuTime_ms = (timing.renderTime_ms + timing.upscaleTime_ms) / config->frames;
    result->inputLag_ms = result->gpuTime_ms * 0.5f;
    
    free(output);
    
    return result;
}

void TSSBenchmark_CompareResults(
    TSSBenchmarkResult* tssResult,
    TSSBenchmarkResult* fsr2Result,
    TSSBenchmarkStats* outComparison
) {
    if (!tssResult || !fsr2Result || !outComparison) return;
    
    outComparison->psnr = tssResult->psnr - fsr2Result->psnr;
    outComparison->ssim = tssResult->ssim - fsr2Result->ssim;
    outComparison->fps = tssResult->fps - fsr2Result->fps;
}

void TSSBenchmark_GenerateReport(
    TSSBenchmarkResult* results,
    int count,
    const char* filename
) {
    if (!results || count <= 0 || !filename) return;
    
    FILE* fp = fopen(filename, "w");
    if (!fp) return;
    
    fprintf(fp, "TSS Benchmark Report\n");
    fprintf(fp, "====================\n\n");
    
    const char* modeNames[] = {
        "TSS Ultimate",
        "FSR 2.2",
        "TSS Simple",
        "Bilinear"
    };
    
    int i;
    for (i = 0; i < count; i++) {
        fprintf(fp, "%s:\n", modeNames[i]);
        fprintf(fp, "  PSNR: %.2f dB\n", results[i].psnr);
        fprintf(fp, "  SSIM: %.4f\n", results[i].ssim);
        fprintf(fp, "  MS-SSIM: %.4f\n", results[i].ms_ssim);
        fprintf(fp, "  RMSE: %.6f\n", results[i].rmse);
        fprintf(fp, "  FPS: %.1f\n", results[i].fps);
        fprintf(fp, "  Frame Time: %.2f ms\n", results[i].frameTime_ms);
        fprintf(fp, "  GPU Time: %.2f ms\n", results[i].gpuTime_ms);
        fprintf(fp, "\n");
    }
    
    fclose(fp);
}

void TSSBenchmark_LogResults(TSSBenchmarkResult* result) {
    if (!result) return;
    
    printf("=== Benchmark Results ===\n");
    printf("PSNR:    %.2f dB\n", result->psnr);
    printf("SSIM:    %.4f\n", result->ssim);
    printf("MS-SSIM: %.4f\n", result->ms_ssim);
    printf("RMSE:    %.6f\n", result->rmse);
    printf("FPS:     %.1f\n", result->fps);
    printf("Frame Time: %.2f ms\n", result->frameTime_ms);
    printf("GPU Time:   %.2f ms\n", result->gpuTime_ms);
    printf("Input Lag:  %.2f ms\n", result->inputLag_ms);
}

void TSSBenchmark_StartFrame(void) {
#if defined(_WIN32) || defined(_WIN64)
    QueryPerformanceCounter(&g_PerfStart);
#endif
}

void TSSBenchmark_EndFrame(TSSFrameTiming* outTiming) {
    if (!outTiming) return;
    
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    
    float elapsed = (float)(end.QuadPart - g_PerfStart.QuadPart) * 1000.0f / (float)g_PerfFreq.QuadPart;
    outTiming->totalTime_ms = elapsed;
}

void TSSBenchmark_Log(TSSLogLevel level, const char* message) {
    const char* levelStr = "INFO";
    switch (level) {
        case TSS_LOG_WARNING: levelStr = "WARN"; break;
        case TSS_LOG_ERROR: levelStr = "ERROR"; break;
        default: break;
    }
    printf("[%s] %s\n", levelStr, message);
}

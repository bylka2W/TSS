#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "TSSMetrics.h"
#include "TSSAntiAliasing.h"
#include "TSSTransform3D.h"

#define BENCHMARK_WIDTH 1920
#define BENCHMARK_HEIGHT 1080
#define BENCHMARK_FRAMES 100

static float* g_referenceFrame = NULL;
static float* g_testFrame = NULL;
static float* g_outputFrame = NULL;
static uint32_t g_width = BENCHMARK_WIDTH;
static uint32_t g_height = BENCHMARK_HEIGHT;
static uint32_t g_channels = 4;

static float TimeElapsed_ms(LARGE_INTEGER start, LARGE_INTEGER end, LARGE_INTEGER freq) {
    return (float)(end.QuadPart - start.QuadPart) * 1000.0f / (float)freq.QuadPart;
}

static void GenerateReferenceFrame(float* frame, uint32_t width, uint32_t height, uint32_t channels, int frameNum) {
    uint32_t x, y, c;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            uint32_t idx = (y * width + x) * channels;
            
            float fx = (float)x / width;
            float fy = (float)y / height;
            
            float wave1 = sinf(fx * 20.0f + frameNum * 0.1f) * 0.5f + 0.5f;
            float wave2 = sinf(fy * 15.0f + frameNum * 0.08f) * 0.5f + 0.5f;
            float wave3 = sinf((fx + fy) * 10.0f + frameNum * 0.05f) * 0.5f + 0.5f;
            
            float pattern = (wave1 + wave2 + wave3) / 3.0f;
            
            float gradient = sqrtf(fx * fx + fy * fy);
            float gradientPattern = sinf(gradient * 30.0f + frameNum * 0.15f) * 0.5f + 0.5f;
            
            float value = pattern * 0.7f + gradientPattern * 0.3f;
            
            for (c = 0; c < 3; c++) {
                float colorMod = 1.0f + 0.3f * sinf((float)c + frameNum * 0.02f);
                frame[idx + c] = TSSMetrics_Clamp(value * colorMod, 0.0f, 1.0f);
            }
            frame[idx + 3] = 1.0f;
        }
    }
}

static void GenerateTestFrameWithFG(float* frame, const float* reference, uint32_t width, uint32_t height, uint32_t channels, int frameNum) {
    uint32_t x, y, c;
    float blurKernel[5] = {0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f};
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            uint32_t idx = (y * width + x) * channels;
            
            float motionX = sinf(frameNum * 0.03f) * 2.0f;
            float motionY = cosf(frameNum * 0.025f) * 1.5f;
            
            int srcX = (int)((float)x - motionX);
            int srcY = (int)((float)y - motionY);
            
            srcX = (srcX < 0) ? 0 : ((srcX >= (int)width) ? ((int)width - 1) : srcX);
            srcY = (srcY < 0) ? 0 : ((srcY >= (int)height) ? ((int)height - 1) : srcY);
            
            uint32_t srcIdx = (srcY * width + srcX) * channels;
            
            for (c = 0; c < channels; c++) {
                frame[idx + c] = reference[srcIdx + c];
            }
        }
    }
    
    for (y = 2; y < height - 2; y++) {
        for (x = 2; x < width - 2; x++) {
            uint32_t idx = (y * width + x) * channels;
            
            float blurredR = 0, blurredG = 0, blurredB = 0;
            int kx, ky;
            
            for (ky = -2; ky <= 2; ky++) {
                for (kx = -2; kx <= 2; kx++) {
                    uint32_t sampleIdx = ((y + ky) * width + (x + kx)) * channels;
                    float weight = blurKernel[ky + 2] * blurKernel[kx + 2];
                    blurredR += frame[sampleIdx] * weight;
                    blurredG += frame[sampleIdx + 1] * weight;
                    blurredB += frame[sampleIdx + 2] * weight;
                }
            }
            
            float motion = fabsf(sinf(frameNum * 0.03f) * 2.0f) + fabsf(cosf(frameNum * 0.025f) * 1.5f);
            float blendFactor = TSSMetrics_Clamp(motion * 0.3f, 0.0f, 0.2f);
            
            frame[idx] = frame[idx] * (1.0f - blendFactor) + blurredR * blendFactor;
            frame[idx + 1] = frame[idx + 1] * (1.0f - blendFactor) + blurredG * blendFactor;
            frame[idx + 2] = frame[idx + 2] * (1.0f - blendFactor) + blurredB * blendFactor;
        }
    }
}

static void GenerateTestFrameTSS(float* frame, const float* reference, uint32_t width, uint32_t height, uint32_t channels, int frameNum, float alpha) {
    uint32_t x, y, c;
    
    float motionX = sinf(frameNum * 0.03f) * 2.0f;
    float motionY = cosf(frameNum * 0.025f) * 1.5f;
    
    float prevMotionX = sinf((frameNum - 1) * 0.03f) * 2.0f;
    float prevMotionY = cosf((frameNum - 1) * 0.025f) * 1.5f;
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            uint32_t idx = (y * width + x) * channels;
            
            float currX = (float)x - motionX;
            float currY = (float)y - motionY;
            
            int currSrcX = (int)currX;
            int currSrcY = (int)currY;
            currSrcX = (currSrcX < 0) ? 0 : ((currSrcX >= (int)width) ? ((int)width - 1) : currSrcX);
            currSrcY = (currSrcY < 0) ? 0 : ((currSrcY >= (int)height) ? ((int)height - 1) : currSrcY);
            
            float prevX = (float)x - prevMotionX;
            float prevY = (float)y - prevMotionY;
            
            int prevSrcX = (int)prevX;
            int prevSrcY = (int)prevY;
            prevSrcX = (prevSrcX < 0) ? 0 : ((prevSrcX >= (int)width) ? ((int)width - 1) : prevSrcX);
            prevSrcY = (prevSrcY < 0) ? 0 : ((prevSrcY >= (int)height) ? ((int)height - 1) : prevSrcY);
            
            uint32_t currSrcIdx = (currSrcY * width + currSrcX) * channels;
            uint32_t prevSrcIdx = (prevSrcY * width + prevSrcX) * channels;
            
            float exactX = currX - currSrcX;
            float exactY = currY - currSrcY;
            
            for (c = 0; c < 3; c++) {
                float currVal = reference[currSrcIdx + c];
                float prevVal = reference[prevSrcIdx + c];
                
                float interpolated = prevVal + (currVal - prevVal) * alpha;
                
                frame[idx + c] = TSSMetrics_Clamp(interpolated, 0.0f, 1.0f);
            }
            frame[idx + 3] = 1.0f;
        }
    }
}

static void PrintResults(TSSMetricsResult result, TSSMultiScaleMetrics msResult, float totalTime, int frameCount) {
    printf("\n");
    printf("=== TSS Frame Generation vs Reference - Metrics ===\n");
    printf("\n");
    printf("Overall Metrics:\n");
    printf("  PSNR: %.2f dB\n", result.psnr);
    printf("  SSIM: %.4f\n", result.ssim);
    printf("  MS-SSIM: %.4f\n", result.ms_ssim);
    printf("  RMSE: %.6f\n", result.rmse);
    printf("  MAE: %.6f\n", result.mae);
    printf("  Max Error: %.6f\n", result.max_error);
    printf("\n");
    
    printf("Multi-Scale SSIM:\n");
    printf("  1x (Native):    %.4f\n", msResult.ssim1x);
    printf("  2x (Upscaled):  %.4f\n", msResult.ssim2x);
    printf("  4x (Upscaled):  %.4f\n", msResult.ssim4x);
    printf("  8x (Upscaled):  %.4f\n", msResult.ssim8x);
    printf("\n");
    
    printf("Multi-Scale PSNR:\n");
    printf("  1x (Native):    %.2f dB\n", msResult.psnr1x);
    printf("  2x (Upscaled):  %.2f dB\n", msResult.psnr2x);
    printf("  4x (Upscaled):  %.2f dB\n", msResult.psnr4x);
    printf("  8x (Upscaled):  %.2f dB\n", msResult.psnr8x);
    printf("\n");
    
    printf("Performance:\n");
    printf("  Total Time:     %.2f ms\n", totalTime);
    printf("  Avg per Frame:  %.2f ms\n", totalTime / frameCount);
    printf("  FPS (metrics):  %.1f frames/sec\n", frameCount * 1000.0f / totalTime);
    printf("\n");
    
    if (result.ssim >= 0.95f) {
        printf("Quality: EXCELLENT (>0.95 SSIM)\n");
    } else if (result.ssim >= 0.90f) {
        printf("Quality: GOOD (0.90-0.95 SSIM)\n");
    } else if (result.ssim >= 0.80f) {
        printf("Quality: ACCEPTABLE (0.80-0.90 SSIM)\n");
    } else {
        printf("Quality: POOR (<0.80 SSIM)\n");
    }
}

static void RunBenchmark(void) {
    printf("TSS Frame Generation Metrics Benchmark\n");
    printf("=======================================\n");
    printf("Resolution: %dx%d\n", g_width, g_height);
    printf("Frames: %d\n", BENCHMARK_FRAMES);
    printf("Channels: %d\n\n", g_channels);
    
    g_referenceFrame = (float*)malloc(g_width * g_height * g_channels * sizeof(float));
    g_testFrame = (float*)malloc(g_width * g_height * g_channels * sizeof(float));
    g_outputFrame = (float*)malloc(g_width * g_height * g_channels * sizeof(float));
    
    if (!g_referenceFrame || !g_testFrame || !g_outputFrame) {
        printf("ERROR: Failed to allocate memory\n");
        return;
    }
    
    TSSAntiAliasing* aa = TSSAA_Create(TSS_AA_TSSAA);
    TSSMetricsConfig config = {0};
    config.blockSize = 8;
    config.stride = 8;
    config.ssimWindowSize = 11.0f;
    config.gaussianSigma = 1.5f;
    config.k1 = 0.01f;
    config.k2 = 0.03f;
    config.useSSIM = true;
    config.usePSNR = true;
    config.useMS_SSIM = true;
    
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    
    TSSMetricsResult totalResult = {0};
    float totalPSNR = 0.0f, totalSSIM = 0.0f, totalMSSSIM = 0.0f;
    
    QueryPerformanceCounter(&start);
    
    int frame;
    for (frame = 0; frame < BENCHMARK_FRAMES; frame++) {
        float alpha = 0.5f;
        
        GenerateReferenceFrame(g_referenceFrame, g_width, g_height, g_channels, frame);
        
        GenerateTestFrameTSS(g_testFrame, g_referenceFrame, g_width, g_height, g_channels, frame, alpha);
        
        TSSMetricsResult result = TSSMetrics_CalculateFloat(
            g_referenceFrame, g_testFrame, g_width, g_height, g_channels, &config
        );
        
        totalPSNR += result.psnr;
        totalSSIM += result.ssim;
        totalMSSSIM += result.ms_ssim;
        
        totalResult.rmse += result.rmse;
        totalResult.mae += result.mae;
        totalResult.max_error += result.max_error;
        
        if (frame % 20 == 0) {
            printf("Frame %d/%d: PSNR=%.2f dB, SSIM=%.4f, MS-SSIM=%.4f\n",
                   frame + 1, BENCHMARK_FRAMES, result.psnr, result.ssim, result.ms_ssim);
        }
    }
    
    QueryPerformanceCounter(&end);
    float totalTime = TimeElapsed_ms(start, end, freq);
    
    TSSMetricsResult avgResult;
    avgResult.psnr = totalPSNR / BENCHMARK_FRAMES;
    avgResult.ssim = totalSSIM / BENCHMARK_FRAMES;
    avgResult.ms_ssim = totalMSSSIM / BENCHMARK_FRAMES;
    avgResult.rmse = totalResult.rmse / BENCHMARK_FRAMES;
    avgResult.mae = totalResult.mae / BENCHMARK_FRAMES;
    avgResult.max_error = totalResult.max_error / BENCHMARK_FRAMES;
    
    TSSMultiScaleMetrics msResult = TSSMetrics_CalculateMultiScale(
        g_referenceFrame, g_testFrame, g_width, g_height
    );
    
    PrintResults(avgResult, msResult, totalTime, BENCHMARK_FRAMES);
    
    TSSDetailedSSIM detailed = TSSMetrics_DetailedSSIM(
        g_referenceFrame, g_testFrame, g_width, g_height, g_width * g_channels
    );
    
    printf("Detailed SSIM Components:\n");
    printf("  Luminance:  %.4f\n", detailed.luminance);
    printf("  Contrast:   %.4f\n", detailed.contrast);
    printf("  Structure:  %.4f\n", detailed.structure);
    printf("\n");
    
    TSSAA_Destroy(aa);
    
    free(g_referenceFrame);
    free(g_testFrame);
    free(g_outputFrame);
    
    printf("Benchmark complete.\n");
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    
    printf("Starting TSS Metrics Benchmark...\n\n");
    
    RunBenchmark();
    
    printf("\nPress Enter to exit...");
    getchar();
    
    return 0;
}

#ifndef TSS_METRICS_H
#define TSS_METRICS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_METRICS_MAX_BLOCK_SIZE 8

typedef struct {
    float psnr;
    float ssim;
    float ms_ssim;
    float msssim;
    float rmse;
    float mae;
    float max_error;
    float avg_error;
} TSSMetricsResult;

typedef struct {
    float ssim;
    float contrast;
    float structure;
    float luminance;
} TSSBlockMetric;

typedef struct {
    float* histogram;
    int binCount;
    float minValue;
    float maxValue;
} TSSHistogram;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    float mean;
    float stddev;
    float min;
    float max;
    float range;
} TSSImageStats;

typedef struct {
    uint32_t blockSize;
    uint32_t stride;
    float ssimWindowSize;
    float gaussianSigma;
    float k1;
    float k2;
    bool useSSIM;
    bool usePSNR;
    bool useMS_SSIM;
} TSSMetricsConfig;

TSSMetricsResult TSSMetrics_Calculate(
    const uint8_t* img1,
    const uint8_t* img2,
    uint32_t width,
    uint32_t height,
    uint32_t channels,
    TSSMetricsConfig* config
);

TSSMetricsResult TSSMetrics_CalculateFloat(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t channels,
    TSSMetricsConfig* config
);

float TSSMetrics_CalculatePSNR(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t channels
);

float TSSMetrics_CalculateSSIM(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t stride,
    uint32_t blockSize,
    float k1,
    float k2
);

float TSSMetrics_CalculateMSSSIM(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t channels
);

TSSImageStats TSSMetrics_ImageStats(const float* img, uint32_t width, uint32_t height, uint32_t channels);

TSSHistogram* TSSMetrics_CreateHistogram(const float* img, uint32_t width, uint32_t height, int binCount);
void TSSMetrics_DestroyHistogram(TSSHistogram* hist);
void TSSMetrics_ComputeHistogram(TSSHistogram* hist, const float* img, uint32_t width, uint32_t height);
float TSSMetrics_HistogramMSE(const TSSHistogram* hist1, const TSSHistogram* hist2);

float TSSMetrics_CalculateMAE(const float* img1, const float* img2, uint32_t width, uint32_t height, uint32_t channels);
float TSSMetrics_CalculateRMSE(const float* img1, const float* img2, uint32_t width, uint32_t height, uint32_t channels);

typedef struct {
    float ssim_map;
    float contrast_map;
    float structure_map;
} TSSDetailedSSIM;

TSSDetailedSSIM TSSMetrics_DetailedSSIM(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t stride
);

void TSSMetrics_WriteSSIMMap(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t stride,
    const char* filename
);

typedef struct {
    float psnr_r;
    float psnr_g;
    float psnr_b;
    float psnr_y;
    float psnr_u;
    float psnr_v;
} TSSColorMetrics;

TSSColorMetrics TSSMetrics_CalculateColorPSNR(
    const uint8_t* img1,
    const uint8_t* img2,
    uint32_t width,
    uint32_t height
);

void TSSMetrics_Benchmark(
    const float* reference,
    const float* test,
    uint32_t width,
    uint32_t height,
    uint32_t channels,
    TSSMetricsConfig* config,
    TSSMetricsResult* result
);

typedef struct {
    float ssim1x;
    float ssim2x;
    float ssim4x;
    float ssim8x;
    float psnr1x;
    float psnr2x;
    float psnr4x;
    float psnr8x;
} TSSMultiScaleMetrics;

TSSMultiScaleMetrics TSSMetrics_CalculateMultiScale(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height
);

#ifdef __cplusplus
}
#endif

#endif

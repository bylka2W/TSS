#include "TSSMetrics.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

static float TSSMetrics_Gaussian(float x, float sigma) {
    float exponent = -(x * x) / (2.0f * sigma * sigma);
    return expf(exponent) / (sigma * sqrtf(2.0f * M_PI_F));
}

static void TSSMetrics_CreateGaussianKernel(float* kernel, int size, float sigma) {
    int half = size / 2;
    float sum = 0.0f;
    int i;
    for (i = 0; i < size; i++) {
        float x = (float)(i - half);
        kernel[i] = TSSMetrics_Gaussian(x, sigma);
        sum += kernel[i];
    }
    for (i = 0; i < size; i++) {
        kernel[i] /= sum;
    }
}

static float TSSMetrics_Clamp(float x, float minVal, float maxVal) {
    return (x < minVal) ? minVal : (x > maxVal) ? maxVal : x;
}

static float TSSMetrics_Luminance(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

TSSMetricsResult TSSMetrics_Calculate(
    const uint8_t* img1,
    const uint8_t* img2,
    uint32_t width,
    uint32_t height,
    uint32_t channels,
    TSSMetricsConfig* config
) {
    TSSMetricsResult result = {0};
    if (!img1 || !img2) return result;
    
    float* img1f = (float*)malloc(width * height * channels * sizeof(float));
    float* img2f = (float*)malloc(width * height * channels * sizeof(float));
    
    uint32_t i;
    for (i = 0; i < width * height * channels; i++) {
        img1f[i] = img1[i] / 255.0f;
        img2f[i] = img2[i] / 255.0f;
    }
    
    result = TSSMetrics_CalculateFloat(img1f, img2f, width, height, channels, config);
    
    free(img1f);
    free(img2f);
    
    return result;
}

TSSMetricsResult TSSMetrics_CalculateFloat(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t channels,
    TSSMetricsConfig* config
) {
    TSSMetricsResult result = {0};
    if (!img1 || !img2 || !config) return result;
    
    uint32_t pixelCount = width * height;
    uint32_t totalSamples = pixelCount * channels;
    
    float mse = 0.0f;
    float maxVal = 1.0f;
    float sumAbsError = 0.0f;
    float maxError = 0.0f;
    
    uint32_t i;
    for (i = 0; i < totalSamples; i++) {
        float diff = img1[i] - img2[i];
        float absError = fabsf(diff);
        
        mse += diff * diff;
        sumAbsError += absError;
        
        if (absError > maxError) maxError = absError;
    }
    mse /= (float)totalSamples;
    
    if (mse > 1e-10f) {
        result.psnr = 10.0f * log10f((maxVal * maxVal) / mse);
    } else {
        result.psnr = 999.0f;
    }
    
    result.rmse = sqrtf(mse);
    result.mae = sumAbsError / (float)totalSamples;
    result.max_error = maxError;
    result.avg_error = result.rmse;
    
    if (config->useSSIM) {
        result.ssim = TSSMetrics_CalculateSSIM(
            img1, img2, width, height, width * channels,
            config->blockSize, config->k1, config->k2
        );
    }
    
    if (config->useMS_SSIM) {
        result.ms_ssim = TSSMetrics_CalculateMSSSIM(img1, img2, width, height, channels);
    }
    
    return result;
}

float TSSMetrics_CalculatePSNR(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t channels
) {
    uint32_t totalSamples = width * height * channels;
    float mse = 0.0f;
    
    uint32_t i;
    for (i = 0; i < totalSamples; i++) {
        float diff = img1[i] - img2[i];
        mse += diff * diff;
    }
    mse /= (float)totalSamples;
    
    if (mse < 1e-10f) return 999.0f;
    
    return 10.0f * log10f(1.0f / mse);
}

float TSSMetrics_CalculateSSIM(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t stride,
    uint32_t blockSize,
    float k1,
    float k2
) {
    if (!img1 || !img2) return 0.0f;
    
    float C1 = (k1 * 1.0f) * (k1 * 1.0f);
    float C2 = (k2 * 1.0f) * (k2 * 1.0f);
    
    int windowSize = 8;
    float gaussianKernel[8];
    TSSMetrics_CreateGaussianKernel(gaussianKernel, windowSize, 1.5f);
    
    float totalSSIM = 0.0f;
    int sampleCount = 0;
    
    uint32_t x, y, wx, wy;
    for (y = windowSize; y < height - windowSize; y += blockSize) {
        for (x = windowSize; x < width - windowSize; x += blockSize) {
            float sum1 = 0.0f, sum2 = 0.0f;
            float sum11 = 0.0f, sum22 = 0.0f, sum12 = 0.0f;
            
            for (wy = 0; wy < (uint32_t)windowSize; wy++) {
                for (wx = 0; wx < (uint32_t)windowSize; wx++) {
                    uint32_t idx1 = (y + wy) * stride + (x + wx) * 4;
                    uint32_t idx2 = (y + wy) * stride + (x + wx) * 4;
                    
                    float r1 = img1[idx1];
                    float r2 = img2[idx2];
                    
                    float mu1 = r1;
                    float mu2 = r2;
                    
                    sum1 += mu1 * gaussianKernel[wy] * gaussianKernel[wx];
                    sum2 += mu2 * gaussianKernel[wy] * gaussianKernel[wx];
                }
            }
            
            float variance1 = 0.0f, variance2 = 0.0f, covariance = 0.0f;
            
            for (wy = 0; wy < (uint32_t)windowSize; wy++) {
                for (wx = 0; wx < (uint32_t)windowSize; wx++) {
                    uint32_t idx1 = (y + wy) * stride + (x + wx) * 4;
                    uint32_t idx2 = (y + wy) * stride + (x + wx) * 4;
                    
                    float r1 = img1[idx1];
                    float r2 = img2[idx2];
                    
                    variance1 += (r1 - sum1) * (r1 - sum1) * gaussianKernel[wy] * gaussianKernel[wx];
                    variance2 += (r2 - sum2) * (r2 - sum2) * gaussianKernel[wy] * gaussianKernel[wx];
                    covariance += (r1 - sum1) * (r2 - sum2) * gaussianKernel[wy] * gaussianKernel[wx];
                }
            }
            
            float numerator = (2.0f * sum1 * sum2 + C1) * (2.0f * covariance + C2);
            float denominator = (sum1 * sum1 + sum2 * sum2 + C1) * (variance1 + variance2 + C2);
            
            float ssim = numerator / denominator;
            
            totalSSIM += ssim;
            sampleCount++;
        }
    }
    
    if (sampleCount > 0) {
        return totalSSIM / (float)sampleCount;
    }
    
    return 0.0f;
}

static float TSSMetrics_SSIMChannel(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t channels,
    float scale
) {
    uint32_t scaledWidth = width / scale;
    uint32_t scaledHeight = height / scale;
    uint32_t scaledStride = scaledWidth * channels;
    
    float C1 = 0.01f * 0.01f;
    float C2 = 0.03f * 0.03f;
    
    float totalSSIM = 0.0f;
    int samples = 0;
    
    uint32_t x, y;
    for (y = 1; y < scaledHeight - 1; y++) {
        for (x = 1; x < scaledWidth - 1; x++) {
            uint32_t sx = x * scale;
            uint32_t sy = y * scale;
            
            float sum1 = 0.0f, sum2 = 0.0f;
            float sum11 = 0.0f, sum22 = 0.0f, sum12 = 0.0f;
            int count = 0;
            
            uint32_t dx, dy;
            for (dy = 0; dy < scale; dy++) {
                for (dx = 0; dx < scale; dx++) {
                    uint32_t idx1 = ((sy + dy) * width + (sx + dx)) * channels;
                    uint32_t idx2 = ((sy + dy) * width + (sx + dx)) * channels;
                    
                    sum1 += img1[idx1];
                    sum2 += img2[idx2];
                    sum11 += img1[idx1] * img1[idx1];
                    sum22 += img2[idx2] * img2[idx2];
                    sum12 += img1[idx1] * img2[idx2];
                    count++;
                }
            }
            
            float n = (float)count;
            float mu1 = sum1 / n;
            float mu2 = sum2 / n;
            float sigma1Sq = sum11 / n - mu1 * mu1;
            float sigma2Sq = sum22 / n - mu2 * mu2;
            float sigma12 = sum12 / n - mu1 * mu2;
            
            float numerator = (2.0f * mu1 * mu2 + C1) * (2.0f * sigma12 + C2);
            float denominator = (mu1 * mu1 + mu2 * mu2 + C1) * (sigma1Sq + sigma2Sq + C2);
            
            totalSSIM += numerator / denominator;
            samples++;
        }
    }
    
    return (samples > 0) ? (totalSSIM / samples) : 0.0f;
}

float TSSMetrics_CalculateMSSSIM(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t channels
) {
    float ssim1 = TSSMetrics_SSIMChannel(img1, img2, width, height, channels, 1);
    float ssim2 = TSSMetrics_SSIMChannel(img1, img2, width, height, channels, 2);
    float ssim4 = TSSMetrics_SSIMChannel(img1, img2, width, height, channels, 4);
    
    return powf(ssim1, 0.5798f) * powf(ssim2, 0.2846f) * powf(ssim4, 0.1356f);
}

TSSImageStats TSSMetrics_ImageStats(const float* img, uint32_t width, uint32_t height, uint32_t channels) {
    TSSImageStats stats = {0};
    if (!img) return stats;
    
    stats.width = width;
    stats.height = height;
    stats.channels = channels;
    
    uint32_t totalSamples = width * height * channels;
    
    float sum = 0.0f;
    float minVal = img[0];
    float maxVal = img[0];
    
    uint32_t i;
    for (i = 0; i < totalSamples; i++) {
        float val = img[i];
        sum += val;
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }
    
    stats.mean = sum / (float)totalSamples;
    stats.min = minVal;
    stats.max = maxVal;
    stats.range = maxVal - minVal;
    
    float variance = 0.0f;
    for (i = 0; i < totalSamples; i++) {
        float diff = img[i] - stats.mean;
        variance += diff * diff;
    }
    stats.stddev = sqrtf(variance / (float)totalSamples);
    
    return stats;
}

TSSHistogram* TSSMetrics_CreateHistogram(const float* img, uint32_t width, uint32_t height, int binCount) {
    TSSHistogram* hist = (TSSHistogram*)calloc(1, sizeof(TSSHistogram));
    if (!hist) return NULL;
    
    hist->binCount = binCount;
    hist->minValue = 0.0f;
    hist->maxValue = 1.0f;
    hist->histogram = (float*)calloc(binCount, sizeof(float));
    
    if (!hist->histogram) {
        free(hist);
        return NULL;
    }
    
    return hist;
}

void TSSMetrics_DestroyHistogram(TSSHistogram* hist) {
    if (!hist) return;
    free(hist->histogram);
    free(hist);
}

void TSSMetrics_ComputeHistogram(TSSHistogram* hist, const float* img, uint32_t width, uint32_t height) {
    if (!hist || !img) return;
    
    memset(hist->histogram, 0, hist->binCount * sizeof(float));
    
    uint32_t totalSamples = width * height;
    float binWidth = (hist->maxValue - hist->minValue) / (float)hist->binCount;
    
    uint32_t i;
    for (i = 0; i < totalSamples; i++) {
        int bin = (int)((img[i] - hist->minValue) / binWidth);
        if (bin >= 0 && bin < hist->binCount) {
            hist->histogram[bin]++;
        }
    }
    
    for (i = 0; i < (uint32_t)hist->binCount; i++) {
        hist->histogram[i] /= (float)totalSamples;
    }
}

float TSSMetrics_HistogramMSE(const TSSHistogram* hist1, const TSSHistogram* hist2) {
    if (!hist1 || !hist2 || hist1->binCount != hist2->binCount) return 0.0f;
    
    float mse = 0.0f;
    int i;
    for (i = 0; i < hist1->binCount; i++) {
        float diff = hist1->histogram[i] - hist2->histogram[i];
        mse += diff * diff;
    }
    mse /= (float)hist1->binCount;
    
    return mse;
}

float TSSMetrics_CalculateMAE(const float* img1, const float* img2, uint32_t width, uint32_t height, uint32_t channels) {
    uint32_t totalSamples = width * height * channels;
    float sumAbsError = 0.0f;
    
    uint32_t i;
    for (i = 0; i < totalSamples; i++) {
        sumAbsError += fabsf(img1[i] - img2[i]);
    }
    
    return sumAbsError / (float)totalSamples;
}

float TSSMetrics_CalculateRMSE(const float* img1, const float* img2, uint32_t width, uint32_t height, uint32_t channels) {
    uint32_t totalSamples = width * height * channels;
    float mse = 0.0f;
    
    uint32_t i;
    for (i = 0; i < totalSamples; i++) {
        float diff = img1[i] - img2[i];
        mse += diff * diff;
    }
    mse /= (float)totalSamples;
    
    return sqrtf(mse);
}

TSSDetailedSSIM TSSMetrics_DetailedSSIM(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height,
    uint32_t stride
) {
    TSSDetailedSSIM result = {0, 0, 0, 0};
    if (!img1 || !img2) return result;
    
    float C1 = 0.01f * 0.01f;
    float C2 = 0.03f * 0.03f;
    
    float sum1 = 0.0f, sum2 = 0.0f;
    float sum11 = 0.0f, sum22 = 0.0f, sum12 = 0.0f;
    int sampleCount = 0;
    
    uint32_t x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            uint32_t idx = y * stride + x * 4;
            
            sum1 += img1[idx];
            sum2 += img2[idx];
            sum11 += img1[idx] * img1[idx];
            sum22 += img2[idx] * img2[idx];
            sum12 += img1[idx] * img2[idx];
            sampleCount++;
        }
    }
    
    float n = (float)sampleCount;
    float mu1 = sum1 / n;
    float mu2 = sum2 / n;
    float sigma1Sq = sum11 / n - mu1 * mu1;
    float sigma2Sq = sum22 / n - mu2 * mu2;
    float sigma12 = sum12 / n - mu1 * mu2;
    
    result.luminance = (2.0f * mu1 * mu2 + C1) / (mu1 * mu1 + mu2 * mu2 + C1);
    result.contrast = (2.0f * sqrtf(sigma1Sq * sigma2Sq) + C2) / (sigma1Sq + sigma2Sq + C2);
    result.structure = (sigma12 + C2) / (sqrtf(sigma1Sq) * sqrtf(sigma2Sq) + C2);
    
    result.ssim_map = result.luminance * result.contrast * result.structure;
    
    return result;
}

TSSColorMetrics TSSMetrics_CalculateColorPSNR(
    const uint8_t* img1,
    const uint8_t* img2,
    uint32_t width,
    uint32_t height
) {
    TSSColorMetrics metrics = {0};
    if (!img1 || !img2) return metrics;
    
    float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f;
    float mseR = 0.0f, mseG = 0.0f, mseB = 0.0f;
    float mseY = 0.0f, mseU = 0.0f, mseV = 0.0f;
    
    uint32_t i;
    for (i = 0; i < width * height; i++) {
        float r1 = img1[i * 3] / 255.0f;
        float g1 = img1[i * 3 + 1] / 255.0f;
        float b1 = img1[i * 3 + 2] / 255.0f;
        
        float r2 = img2[i * 3] / 255.0f;
        float g2 = img2[i * 3 + 1] / 255.0f;
        float b2 = img2[i * 3 + 2] / 255.0f;
        
        mseR += (r1 - r2) * (r1 - r2);
        mseG += (g1 - g2) * (g1 - g2);
        mseB += (b1 - b2) * (b1 - b2);
        
        float y1 = 0.299f * r1 + 0.587f * g1 + 0.114f * b1;
        float y2 = 0.299f * r2 + 0.587f * g2 + 0.114f * b2;
        float u1 = -0.169f * r1 - 0.331f * g1 + 0.5f * b1;
        float u2 = -0.169f * r2 - 0.331f * g2 + 0.5f * b2;
        float v1 = 0.5f * r1 - 0.419f * g1 - 0.081f * b1;
        float v2 = 0.5f * r2 - 0.419f * g2 - 0.081f * b2;
        
        mseY += (y1 - y2) * (y1 - y2);
        mseU += (u1 - u2) * (u1 - u2);
        mseV += (v1 - v2) * (v1 - v2);
    }
    
    float n = (float)(width * height);
    mseR /= n; mseG /= n; mseB /= n;
    mseY /= n; mseU /= n; mseV /= n;
    
    metrics.psnr_r = (mseR > 1e-10f) ? 10.0f * log10f(1.0f / mseR) : 999.0f;
    metrics.psnr_g = (mseG > 1e-10f) ? 10.0f * log10f(1.0f / mseG) : 999.0f;
    metrics.psnr_b = (mseB > 1e-10f) ? 10.0f * log10f(1.0f / mseB) : 999.0f;
    metrics.psnr_y = (mseY > 1e-10f) ? 10.0f * log10f(1.0f / mseY) : 999.0f;
    metrics.psnr_u = (mseU > 1e-10f) ? 10.0f * log10f(1.0f / mseU) : 999.0f;
    metrics.psnr_v = (mseV > 1e-10f) ? 10.0f * log10f(1.0f / mseV) : 999.0f;
    
    return metrics;
}

void TSSMetrics_Benchmark(
    const float* reference,
    const float* test,
    uint32_t width,
    uint32_t height,
    uint32_t channels,
    TSSMetricsConfig* config,
    TSSMetricsResult* result
) {
    if (!reference || !test || !config || !result) return;
    
    *result = TSSMetrics_CalculateFloat(reference, test, width, height, channels, config);
}

TSSMultiScaleMetrics TSSMetrics_CalculateMultiScale(
    const float* img1,
    const float* img2,
    uint32_t width,
    uint32_t height
) {
    TSSMultiScaleMetrics metrics = {0};
    
    metrics.ssim1x = TSSMetrics_CalculateSSIM(img1, img2, width, height, width * 4, 8, 0.01f, 0.03f);
    metrics.psnr1x = TSSMetrics_CalculatePSNR(img1, img2, width, height, 4);
    
    if (width >= 2 && height >= 2) {
        uint32_t halfW = width / 2;
        uint32_t halfH = height / 2;
        
        float* half1 = (float*)malloc(halfW * halfH * 4 * sizeof(float));
        float* half2 = (float*)malloc(halfW * halfH * 4 * sizeof(float));
        
        uint32_t x, y;
        for (y = 0; y < halfH; y++) {
            for (x = 0; x < halfW; x++) {
                uint32_t srcIdx = (y * 2 * width + x * 2) * 4;
                uint32_t dstIdx = (y * halfW + x) * 4;
                half1[dstIdx] = img1[srcIdx];
                half1[dstIdx + 1] = img1[srcIdx + 1];
                half1[dstIdx + 2] = img1[srcIdx + 2];
                half1[dstIdx + 3] = img1[srcIdx + 3];
                half2[dstIdx] = img2[srcIdx];
                half2[dstIdx + 1] = img2[srcIdx + 1];
                half2[dstIdx + 2] = img2[srcIdx + 2];
                half2[dstIdx + 3] = img2[srcIdx + 3];
            }
        }
        
        metrics.ssim2x = TSSMetrics_CalculateSSIM(half1, half2, halfW, halfH, halfW * 4, 8, 0.01f, 0.03f);
        metrics.psnr2x = TSSMetrics_CalculatePSNR(half1, half2, halfW, halfH, 4);
        
        if (halfW >= 2 && halfH >= 2) {
            uint32_t quarterW = halfW / 2;
            uint32_t quarterH = halfH / 2;
            
            float* quarter1 = (float*)malloc(quarterW * quarterH * 4 * sizeof(float));
            float* quarter2 = (float*)malloc(quarterW * quarterH * 4 * sizeof(float));
            
            for (y = 0; y < quarterH; y++) {
                for (x = 0; x < quarterW; x++) {
                    uint32_t srcIdx = (y * 2 * halfW + x * 2) * 4;
                    uint32_t dstIdx = (y * quarterW + x) * 4;
                    quarter1[dstIdx] = half1[srcIdx];
                    quarter1[dstIdx + 1] = half1[srcIdx + 1];
                    quarter1[dstIdx + 2] = half1[srcIdx + 2];
                    quarter1[dstIdx + 3] = half1[srcIdx + 3];
                    quarter2[dstIdx] = half2[srcIdx];
                    quarter2[dstIdx + 1] = half2[srcIdx + 1];
                    quarter2[dstIdx + 2] = half2[srcIdx + 2];
                    quarter2[dstIdx + 3] = half2[srcIdx + 3];
                }
            }
            
            metrics.ssim4x = TSSMetrics_CalculateSSIM(quarter1, quarter2, quarterW, quarterH, quarterW * 4, 8, 0.01f, 0.03f);
            metrics.psnr4x = TSSMetrics_CalculatePSNR(quarter1, quarter2, quarterW, quarterH, 4);
            
            if (quarterW >= 2 && quarterH >= 2) {
                uint32_t eighthW = quarterW / 2;
                uint32_t eighthH = quarterH / 2;
                
                float* eighth1 = (float*)malloc(eighthW * eighthH * 4 * sizeof(float));
                float* eighth2 = (float*)malloc(eighthW * eighthH * 4 * sizeof(float));
                
                for (y = 0; y < eighthH; y++) {
                    for (x = 0; x < eighthW; x++) {
                        uint32_t srcIdx = (y * 2 * quarterW + x * 2) * 4;
                        uint32_t dstIdx = (y * eighthW + x) * 4;
                        eighth1[dstIdx] = quarter1[srcIdx];
                        eighth1[dstIdx + 1] = quarter1[srcIdx + 1];
                        eighth1[dstIdx + 2] = quarter1[srcIdx + 2];
                        eighth1[dstIdx + 3] = quarter1[srcIdx + 3];
                        eighth2[dstIdx] = quarter2[srcIdx];
                        eighth2[dstIdx + 1] = quarter2[srcIdx + 1];
                        eighth2[dstIdx + 2] = quarter2[srcIdx + 2];
                        eighth2[dstIdx + 3] = quarter2[srcIdx + 3];
                    }
                }
                
                metrics.ssim8x = TSSMetrics_CalculateSSIM(eighth1, eighth2, eighthW, eighthH, eighthW * 4, 8, 0.01f, 0.03f);
                metrics.psnr8x = TSSMetrics_CalculatePSNR(eighth1, eighth2, eighthW, eighthH, 4);
                
                free(eighth1);
                free(eighth2);
            }
            
            free(quarter1);
            free(quarter2);
        }
        
        free(half1);
        free(half2);
    }
    
    return metrics;
}

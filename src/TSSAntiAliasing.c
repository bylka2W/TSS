#include "TSSAntiAliasing.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static const float TSS_AA_HDR10_MIN = 0.001f;
static const float TSS_AA_HDR10_MAX = 100.0f;

static float TSS_HDR10_EOTF(float x) {
    if (x <= 0.0f) return 0.0f;
    if (x <= 0.0003024f) return x * 54.5f;
    return powf((x + 0.0245786f) / 1.07793f, 2.4f);
}

static float TSS_HDR10_OETF(float x) {
    if (x <= 0.0f) return 0.0f;
    if (x <= 0.0031308f) return x / 12.92f;
    return 1.055f * powf(x, 1.0f / 2.4f) - 0.055f;
}

TSSAntiAliasing* TSSAA_Create(TSSAAType type) {
    TSSAntiAliasing* aa = (TSSAntiAliasing*)calloc(1, sizeof(TSSAntiAliasing));
    if (!aa) return NULL;
    
    aa->type = type;
    aa->config.subpixelOffsetX = 0.0f;
    aa->config.subpixelOffsetY = 0.0f;
    aa->config.mipBias = 0.0f;
    aa->config.sharpenAmount = 0.0f;
    
    aa->taa.currentIndex = 0;
    aa->taa.enable = true;
    aa->taa.useTemporal = true;
    aa->taa.colorWeight = 0.95f;
    aa->taa.spatialWeight = 0.05f;
    aa->taa.centerWeight = 0.5f;
    
    int i;
    for (i = 0; i < TSS_AA_JITTER_COUNT; i++) {
        float phi = (float)i * 2.39996323f;
        float r = sqrtf((float)(i + 1) / (float)TSS_AA_JITTER_COUNT);
        aa->taa.jitterX[i] = cosf(phi) * r;
        aa->taa.jitterY[i] = sinf(phi) * r;
    }
    
    float fxaaWeights[3][3] = {
        {1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f},
        {2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f},
        {1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f}
    };
    
    int r, c;
    for (r = 0; r < 3; r++) {
        for (c = 0; c < 3; c++) {
            aa->fxaa.weights[r][c] = fxaaWeights[r][c];
            aa->fxaa.offsetX[r][c] = (float)(c - 1);
            aa->fxaa.offsetY[r][c] = (float)(r - 1);
        }
    }
    aa->fxaa.radius = 1;
    
    aa->smaa.positions[0] = 0.25f;
    aa->smaa.positions[1] = -0.25f;
    aa->smaa.weights[0] = 0.5f;
    aa->smaa.weights[1] = 0.25f;
    aa->smaa.mipOffsets[0] = 1.0f;
    aa->smaa.mipOffsets[1] = 2.0f;
    
    return aa;
}

void TSSAA_Destroy(TSSAntiAliasing* aa) {
    free(aa);
}

void TSSAA_SetConfig(TSSAntiAliasing* aa, TSSAAConfig* config) {
    if (!aa || !config) return;
    memcpy(&aa->config, config, sizeof(TSSAAConfig));
}

void TSSAA_GetJitter(TSSAntiAliasing* aa, float* outX, float* outY) {
    if (!aa || !outX || !outY) return;
    
    *outX = aa->taa.jitterX[aa->taa.currentIndex] * 0.5f;
    *outY = aa->taa.jitterY[aa->taa.currentIndex] * 0.5f;
    
    aa->taa.currentIndex = (aa->taa.currentIndex + 1) % TSS_AA_JITTER_COUNT;
}

float TSSAA_CalculateLuma(TSSColor color) {
    return 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
}

float TSSAA_CalculateEdgeStrength(float lumaCenter, float lumaN, float lumaS, float lumaE, float lumaW) {
    float edgeN = fabsf(lumaN - lumaCenter);
    float edgeS = fabsf(lumaS - lumaCenter);
    float edgeE = fabsf(lumaE - lumaCenter);
    float edgeW = fabsf(lumaW - lumaCenter);
    
    float edgeH = fabsf(lumaE - lumaW);
    float edgeV = fabsf(lumaN - lumaS);
    
    return fmaxf(edgeH, edgeV) * 2.0f + edgeN + edgeS + edgeE + edgeW;
}

void TSSAA_ApplyFXAA(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int pitch,
    TSSFXAAConfig* config
) {
    if (!src || !dst || !config) return;
    
    int x, y;
    for (y = 1; y < height - 1; y++) {
        for (x = 1; x < width - 1; x++) {
            int idx = y * pitch + x;
            int idxN = (y - 1) * pitch + x;
            int idxS = (y + 1) * pitch + x;
            int idxW = y * pitch + (x - 1);
            int idxE = y * pitch + (x + 1);
            
            float lumaC = src[idx];
            float lumaN = src[idxN];
            float lumaS = src[idxS];
            float lumaW = src[idxW];
            float lumaE = src[idxE];
            
            float edgeStrength = TSSAA_CalculateEdgeStrength(lumaC, lumaN, lumaS, lumaE, lumaW);
            
            if (edgeStrength > 0.05f) {
                float filtered = 0.0f;
                int r, c;
                for (r = 0; r < 3; r++) {
                    for (c = 0; c < 3; c++) {
                        int sampleX = x + (c - 1);
                        int sampleY = y + (r - 1);
                        int sampleIdx = sampleY * pitch + sampleX;
                        filtered += src[sampleIdx] * config->weights[r][c];
                    }
                }
                dst[idx] = (uint8_t)(filtered + 0.5f);
            } else {
                dst[idx] = src[idx];
            }
        }
    }
}

void TSSAA_ApplyFXAA_Float(
    const float* src,
    float* dst,
    int width,
    int height,
    int pitch,
    TSSFXAAConfig* config
) {
    if (!src || !dst || !config) return;
    
    int x, y;
    for (y = 1; y < height - 1; y++) {
        for (x = 1; x < width - 1; x++) {
            int idx = y * pitch + x;
            int idxN = (y - 1) * pitch + x;
            int idxS = (y + 1) * pitch + x;
            int idxW = y * pitch + (x - 1);
            int idxE = y * pitch + (x + 1);
            int idxNE = (y - 1) * pitch + (x + 1);
            int idxNW = (y - 1) * pitch + (x - 1);
            int idxSE = (y + 1) * pitch + (x + 1);
            int idxSW = (y + 1) * pitch + (x - 1);
            
            float lumaC = 0.2126f * src[idx*4] + 0.7152f * src[idx*4+1] + 0.0722f * src[idx*4+2];
            float lumaN = 0.2126f * src[idxN*4] + 0.7152f * src[idxN*4+1] + 0.0722f * src[idxN*4+2];
            float lumaS = 0.2126f * src[idxS*4] + 0.7152f * src[idxS*4+1] + 0.0722f * src[idxS*4+2];
            float lumaW = 0.2126f * src[idxW*4] + 0.7152f * src[idxW*4+1] + 0.0722f * src[idxW*4+2];
            float lumaE = 0.2126f * src[idxE*4] + 0.7152f * src[idxE*4+1] + 0.0722f * src[idxE*4+2];
            
            float edgeH = fabsf(lumaE - lumaW) * 2.0f;
            float edgeV = fabsf(lumaN - lumaS) * 2.0f;
            float edgeStrength = edgeH + edgeV + fabsf(lumaNE - lumaSW) + fabsf(lumaNW - lumaSE);
            
            if (edgeStrength > 0.03f) {
                int r, c;
                for (r = 0; r < 3; r++) {
                    for (c = 0; c < 3; c++) {
                        int sampleX = x + (c - 1);
                        int sampleY = y + (r - 1);
                        int sampleIdx = (sampleY * pitch + sampleX) * 4;
                        
                        float weight = config->weights[r][c];
                        dst[sampleIdx] += (src[sampleIdx] - dst[sampleIdx]) * weight;
                        dst[sampleIdx+1] += (src[sampleIdx+1] - dst[sampleIdx+1]) * weight;
                        dst[sampleIdx+2] += (src[sampleIdx+2] - dst[sampleIdx+2]) * weight;
                    }
                }
            } else {
                dst[idx*4] = src[idx*4];
                dst[idx*4+1] = src[idx*4+1];
                dst[idx*4+2] = src[idx*4+2];
                dst[idx*4+3] = src[idx*4+3];
            }
        }
    }
}

TSSColor TSSAA_ApplyTAASample(TSSColor* samples, int count, float centerWeight) {
    TSSColor result = {0, 0, 0, 0};
    if (!samples || count == 0) return result;
    
    float totalWeight = 0.0f;
    int i;
    for (i = 0; i < count; i++) {
        float weight = (i == 0) ? centerWeight : (1.0f - centerWeight) / (float)(count - 1);
        result.r += samples[i].r * weight;
        result.g += samples[i].g * weight;
        result.b += samples[i].b * weight;
        result.a += samples[i].a * weight;
        totalWeight += weight;
    }
    
    if (totalWeight > 0.0f) {
        result.r /= totalWeight;
        result.g /= totalWeight;
        result.b /= totalWeight;
        result.a /= totalWeight;
    }
    
    return result;
}

void TSSAA_GenerateJitterPattern(float* outX, float* outY, int count, int width, int height) {
    if (!outX || !outY || count <= 0) return;
    
    int i;
    for (i = 0; i < count; i++) {
        float fi = (float)i;
        outX[i] = (fmodf(fi * 0.7548776662f, 1.0f) - 0.5f) * 2.0f;
        outY[i] = (fmodf(fi * 0.5698402900f, 1.0f) - 0.5f) * 2.0f;
    }
}

float TSSAA_CalculateSharpenWeight(float center, float neighbor, float amount) {
    float diff = center - neighbor;
    return 0.5f + amount * diff;
}

void TSSAA_Sharpen(
    const float* src,
    float* dst,
    int width,
    int height,
    int pitch,
    float amount
) {
    if (!src || !dst || amount <= 0.0f) return;
    
    int x, y;
    for (y = 1; y < height - 1; y++) {
        for (x = 1; x < width - 1; x++) {
            int idx = (y * pitch + x) * 4;
            int idxN = ((y - 1) * pitch + x) * 4;
            int idxS = ((y + 1) * pitch + x) * 4;
            int idxW = (y * pitch + (x - 1)) * 4;
            int idxE = (y * pitch + (x + 1)) * 4;
            
            int c;
            for (c = 0; c < 3; c++) {
                float center = src[idx + c];
                float blur = (src[idxN + c] + src[idxS + c] + src[idxW + c] + src[idxE + c]) * 0.25f;
                float highFreq = center - blur;
                
                float sharpen = center + highFreq * amount;
                dst[idx + c] = (sharpen < 0.0f) ? 0.0f : (sharpen > 1.0f) ? 1.0f : sharpen;
            }
            dst[idx + 3] = src[idx + 3];
        }
    }
}

void TSSPostProcess_Apply(
    const float* src,
    float* dst,
    int width,
    int height,
    TSSPostProcess* pp
) {
    if (!src || !dst || !pp) return;
    
    int x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            
            float r = src[idx];
            float g = src[idx + 1];
            float b = src[idx + 2];
            
            r *= pp->exposure;
            g *= pp->exposure;
            b *= pp->exposure;
            
            float luminance = 0.2126f * r + 0.7152f * g + 0.0722f * b;
            float shadowFactor = (1.0f - pp->shadows) * 2.0f;
            float highlightFactor = pp->highlights * 2.0f;
            
            if (luminance < 0.5f) {
                float factor = 1.0f + (0.5f - luminance) * shadowFactor;
                r *= factor;
                g *= factor;
                b *= factor;
            } else {
                float factor = 1.0f + (luminance - 0.5f) * highlightFactor;
                r *= factor;
                g *= factor;
                b *= factor;
            }
            
            float gray = 0.299f * r + 0.587f * g + 0.114f * b;
            r = gray + (r - gray) * pp->saturation;
            g = gray + (g - gray) * pp->saturation;
            b = gray + (b - gray) * pp->saturation;
            
            r = 1.0f / (1.0f + pp->contrast / (r + 0.0001f));
            g = 1.0f / (1.0f + pp->contrast / (g + 0.0001f));
            b = 1.0f / (1.0f + pp->contrast / (b + 0.0001f));
            
            dst[idx] = r;
            dst[idx + 1] = g;
            dst[idx + 2] = b;
            dst[idx + 3] = src[idx + 3];
        }
    }
}

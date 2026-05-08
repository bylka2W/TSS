#include "TSSMotionVectors.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

TSSMotionVectorField* TSSMV_CreateField(int width, int height, int format) {
    TSSMotionVectorField* field = (TSSMotionVectorField*)calloc(1, sizeof(TSSMotionVectorField));
    if (!field) return NULL;
    
    field->width = width;
    field->height = height;
    field->stride = width;
    field->format = format;
    field->vectors = (TSSPixelMotionVector*)calloc(width * height, sizeof(TSSPixelMotionVector));
    
    if (!field->vectors) {
        free(field);
        return NULL;
    }
    
    return field;
}

void TSSMV_DestroyField(TSSMotionVectorField* field) {
    if (!field) return;
    free(field->vectors);
    free(field);
}

void TSSMV_ClearField(TSSMotionVectorField* field) {
    if (!field || !field->vectors) return;
    memset(field->vectors, 0, field->width * field->height * sizeof(TSSPixelMotionVector));
}

void TSSMV_SetVector(TSSMotionVectorField* field, int x, int y, float mvX, float mvY, float confidence) {
    if (!field || x < 0 || x >= field->width || y < 0 || y >= field->height) return;
    
    int idx = y * field->stride + x;
    field->vectors[idx].mvX = mvX;
    field->vectors[idx].mvY = mvY;
    field->vectors[idx].confidence = confidence;
    field->vectors[idx].timestamp = 0;
}

TSSPixelMotionVector TSSMV_GetVector(TSSMotionVectorField* field, int x, int y) {
    TSSPixelMotionVector empty = {0, 0, 0, 0};
    if (!field || x < 0 || x >= field->width || y < 0 || y >= field->height) return empty;
    
    return field->vectors[y * field->stride + x];
}

static TSSVec2 TSSMV_WorldToScreen(TSSMat4 vp, TSSVec3 worldPos) {
    TSSVec4 pos4;
    pos4.x = worldPos.x;
    pos4.y = worldPos.y;
    pos4.z = worldPos.z;
    pos4.w = 1.0f;
    
    TSSVec4 clip;
    clip.x = vp.m[0] * pos4.x + vp.m[4] * pos4.y + vp.m[8] * pos4.z + vp.m[12] * pos4.w;
    clip.y = vp.m[1] * pos4.x + vp.m[5] * pos4.y + vp.m[9] * pos4.z + vp.m[13] * pos4.w;
    clip.z = vp.m[2] * pos4.x + vp.m[6] * pos4.y + vp.m[10] * pos4.z + vp.m[14] * pos4.w;
    clip.w = vp.m[3] * pos4.x + vp.m[7] * pos4.y + vp.m[11] * pos4.z + vp.m[15] * pos4.w;
    
    TSSVec2 screen;
    screen.x = (clip.x / clip.w) * 0.5f + 0.5f;
    screen.y = (-clip.y / clip.w) * 0.5f + 0.5f;
    
    return screen;
}

void TSSMV_GenerateFromTransform(
    TSSMotionVectorField* field,
    TSSMat4 prevViewProj,
    TSSMat4 currViewProj,
    TSSVec3* positions,
    int count
) {
    if (!field || !positions || count == 0) return;
    
    TSSMV_ClearField(field);
    
    int i;
    for (i = 0; i < count; i++) {
        TSSVec2 prevScreen = TSSMV_WorldToScreen(prevViewProj, positions[i]);
        TSSVec2 currScreen = TSSMV_WorldToScreen(currViewProj, positions[i]);
        
        int sx = (int)(prevScreen.x * field->width);
        int sy = (int)(prevScreen.y * field->height);
        
        if (sx >= 0 && sx < field->width && sy >= 0 && sy < field->height) {
            float mvX = (currScreen.x - prevScreen.x) * field->width;
            float mvY = (currScreen.y - prevScreen.y) * field->height;
            float confidence = 1.0f;
            
            TSSMV_SetVector(field, sx, sy, mvX, mvY, confidence);
        }
    }
}

void TSSMV_GenerateSceneMotionVectors(
    TSSMotionVectorField* field,
    TSSCamera3D* prevCamera,
    TSSCamera3D* currCamera
) {
    if (!field || !prevCamera || !currCamera) return;
    
    TSSMat4 prevVP = TSSMat4_Multiply(prevCamera->projectionMatrix, prevCamera->viewMatrix);
    TSSMat4 currVP = TSSMat4_Multiply(currCamera->projectionMatrix, currCamera->viewMatrix);
    
    TSSVec3 camPos = currCamera->position;
    
    float gridStepX = 4.0f;
    float gridStepY = 4.0f;
    
    int x, y;
    int count = 0;
    TSSVec3* positions = (TSSVec3*)malloc((field->width / (int)gridStepX + 2) * (field->height / (int)gridStepY + 2) * sizeof(TSSVec3));
    
    for (y = 0; y < field->height; y += (int)gridStepY) {
        for (x = 0; x < field->width; x += (int)gridStepX) {
            float screenX = (float)x / field->width;
            float screenY = (float)y / field->height;
            
            TSSVec3 rayOrigin = currCamera->position;
            float tanFov = tanf(currCamera->fov * 0.5f);
            float aspect = currCamera->aspectRatio;
            
            float rayX = (2.0f * screenX - 1.0f) * aspect * tanFov;
            float rayY = (1.0f - 2.0f * screenY) * tanFov;
            
            TSSVec3 rayDir;
            rayDir.x = rayX * currCamera->viewMatrix.m[0] + rayY * currCamera->viewMatrix.m[4] - currCamera->viewMatrix.m[8];
            rayDir.y = rayX * currCamera->viewMatrix.m[1] + rayY * currCamera->viewMatrix.m[5] - currCamera->viewMatrix.m[9];
            rayDir.z = rayX * currCamera->viewMatrix.m[2] + rayY * currCamera->viewMatrix.m[6] - currCamera->viewMatrix.m[10];
            rayDir = TSSVec3_Normalize(rayDir);
            
            float hitT = 10.0f;
            if (rayDir.y < 0) {
                hitT = -rayOrigin.y / rayDir.y;
            }
            
            TSSVec3 hitPos;
            hitPos.x = rayOrigin.x + rayDir.x * hitT;
            hitPos.y = rayOrigin.y + rayDir.y * hitT;
            hitPos.z = rayOrigin.z + rayDir.z * hitT;
            
            positions[count++] = hitPos;
        }
    }
    
    TSSMV_GenerateFromTransform(field, prevCamera->viewMatrix, currCamera->viewMatrix, positions, count);
    
    free(positions);
}

void TSSMV_Dilate(TSSMotionVectorField* field, int radius) {
    if (!field || radius <= 0) return;
    
    TSSPixelMotionVector* temp = (TSSPixelMotionVector*)malloc(
        field->width * field->height * sizeof(TSSPixelMotionVector)
    );
    
    memcpy(temp, field->vectors, field->width * field->height * sizeof(TSSPixelMotionVector));
    
    int x, y, dx, dy;
    for (y = 0; y < field->height; y++) {
        for (x = 0; x < field->width; x++) {
            int idx = y * field->stride + x;
            TSSPixelMotionVector mv = temp[idx];
            
            if (mv.confidence < 0.5f) {
                float sumX = 0, sumY = 0;
                float weightSum = 0;
                int count = 0;
                
                for (dy = -radius; dy <= radius; dy++) {
                    for (dx = -radius; dx <= radius; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        if (nx >= 0 && nx < field->width && ny >= 0 && ny < field->height) {
                            TSSPixelMotionVector neighbor = temp[ny * field->stride + nx];
                            if (neighbor.confidence > 0.5f) {
                                float weight = 1.0f / (1.0f + sqrtf((float)(dx*dx + dy*dy)));
                                sumX += neighbor.mvX * weight;
                                sumY += neighbor.mvY * weight;
                                weightSum += weight;
                                count++;
                            }
                        }
                    }
                }
                
                if (count > 0) {
                    field->vectors[idx].mvX = sumX / weightSum;
                    field->vectors[idx].mvY = sumY / weightSum;
                    field->vectors[idx].confidence = 0.8f;
                }
            }
        }
    }
    
    free(temp);
}

void TSSMV_TemporalReproject(
    TSSMotionVectorField* field,
    TSSMotionVectorCache* cache,
    TSSMat4 prevVP,
    TSSMat4 currVP
) {
    (void)field;
    (void)cache;
    (void)prevVP;
    (void)currVP;
}

void TSSMV_Upscale(
    TSSMotionVectorField* dst,
    TSSMotionVectorField* src,
    float scaleX,
    float scaleY
) {
    if (!dst || !src) return;
    
    TSSMV_ClearField(dst);
    
    int x, y;
    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            TSSPixelMotionVector mv = TSSMV_GetVector(src, x, y);
            
            float dstX = x * scaleX;
            float dstY = y * scaleY;
            
            int ix = (int)dstX;
            int iy = (int)dstY;
            
            float fx = dstX - ix;
            float fy = dstY - iy;
            
            int x1 = (ix < dst->width - 1) ? ix + 1 : ix;
            int y1 = (iy < dst->height - 1) ? iy + 1 : iy;
            ix = (ix < dst->width) ? ix : dst->width - 1;
            iy = (iy < dst->height) ? iy : dst->height - 1;
            
            TSSPixelMotionVector mv00 = TSSMV_GetVector(src, ix, iy);
            TSSPixelMotionVector mv10 = TSSMV_GetVector(src, x1, iy);
            TSSPixelMotionVector mv01 = TSSMV_GetVector(src, ix, y1);
            TSSPixelMotionVector mv11 = TSSMV_GetVector(src, x1, y1);
            
            float outX = mv00.mvX * (1-fx) * (1-fy) + mv10.mvX * fx * (1-fy) +
                        mv01.mvX * (1-fx) * fy + mv11.mvX * fx * fy;
            float outY = mv00.mvY * (1-fx) * (1-fy) + mv10.mvY * fx * (1-fy) +
                        mv01.mvY * (1-fx) * fy + mv11.mvY * fx * fy;
            
            TSSMV_SetVector(dst, ix, iy, outX * scaleX, outY * scaleY, mv.confidence);
        }
    }
}

float TSSMV_GetConfidence(TSSMotionVectorField* field, int x, int y) {
    TSSPixelMotionVector mv = TSSMV_GetVector(field, x, y);
    return mv.confidence;
}

TSSMotionVectorCache* TSSMV_CreateCache(int width, int height, int historyLength) {
    TSSMotionVectorCache* cache = (TSSMotionVectorCache*)calloc(1, sizeof(TSSMotionVectorCache));
    if (!cache) return NULL;
    
    cache->width = width;
    cache->height = height;
    cache->historyLength = historyLength;
    cache->confidenceThreshold = 0.5f;
    
    cache->history = (TSSMotionVectorHistory*)calloc(
        width * height * historyLength, sizeof(TSSMotionVectorHistory)
    );
    
    if (!cache->history) {
        free(cache);
        return NULL;
    }
    
    return cache;
}

void TSSMV_DestroyCache(TSSMotionVectorCache* cache) {
    if (!cache) return;
    free(cache->history);
    free(cache);
}

void TSSMV_CacheUpdate(TSSMotionVectorCache* cache, TSSMotionVectorField* field) {
    if (!cache || !field) return;
    
    int shift = (cache->historyLength - 1) * cache->width * cache->height;
    memmove(cache->history, cache->history + cache->width * cache->height,
            shift * sizeof(TSSMotionVectorHistory));
    
    int x, y;
    int idx = shift;
    for (y = 0; y < field->height; y++) {
        for (x = 0; x < field->width; x++) {
            TSSPixelMotionVector mv = TSSMV_GetVector(field, x, y);
            cache->history[idx].current.x = mv.mvX;
            cache->history[idx].current.y = mv.mvY;
            cache->history[idx].confidence = mv.confidence;
            idx++;
        }
    }
}

TSSMotionVectorHistory* TSSMV_CacheGet(TSSMotionVectorCache* cache, int x, int y) {
    if (!cache || x < 0 || x >= cache->width || y < 0 || y >= cache->height) return NULL;
    
    return &cache->history[(cache->historyLength - 1) * cache->width * cache->height +
                          y * cache->width + x];
}

TSSMVStats TSSMV_CalculateStats(TSSMotionVectorField* field) {
    TSSMVStats stats = {0};
    if (!field) return stats;
    
    int count = 0;
    float totalMagnitude = 0;
    float maxMag = 0;
    float totalConfidence = 0;
    
    int x, y;
    for (y = 0; y < field->height; y++) {
        for (x = 0; x < field->width; x++) {
            TSSPixelMotionVector mv = TSSMV_GetVector(field, x, y);
            
            if (mv.confidence > 0.1f) {
                float mag = sqrtf(mv.mvX * mv.mvX + mv.mvY * mv.mvY);
                totalMagnitude += mag;
                totalConfidence += mv.confidence;
                count++;
                
                if (mag > maxMag) maxMag = mag;
            }
        }
    }
    
    if (count > 0) {
        stats.avgConfidence = totalConfidence / count;
        stats.avgMagnitude = totalMagnitude / count;
        stats.maxMagnitude = maxMag;
        stats.coverage = (float)count / (field->width * field->height);
    }
    
    return stats;
}

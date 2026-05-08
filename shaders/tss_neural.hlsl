#define TSS_NEURAL_VERSION "1.0.0"
#include "tss_common.hlsl"

static const float tss_w_depth = 0.4f;
static const float tss_w_mv = 0.3f;
static const float tss_w_luma = 0.3f;

static const float tss_confidence_threshold = 0.85f;
static const float tss_disocclusion_threshold = 0.1f;
static const float tss_velocity_threshold = 0.05f;

float ComputeDepthTrust(float currentDepth, float historyDepth) {
    if (currentDepth < TSS_EPSILON) return 0.0f;
    
    float depthDiff = abs(currentDepth - historyDepth) / currentDepth;
    float depthTrust = 1.0f - saturate(depthDiff * 10.0f);
    
    return saturate(depthTrust);
}

float ComputeVelocityTrust(float2 mv, float2 prevMV) {
    float mvLen = length(mv);
    float prevMVLen = length(prevMV);
    
    float lenTrust = 1.0f - saturate(mvLen / tss_velocity_threshold);
    
    float2 mvDiff = mv - prevMV;
    float dirTrust = 1.0f - saturate(length(mvDiff) * 5.0f);
    
    float velocityTrust = lenTrust * 0.6f + dirTrust * 0.4f;
    
    return saturate(velocityTrust);
}

float ComputeLuminanceStability(float currY, float histY, float neighborhoodStd) {
    if (abs(histY) < TSS_EPSILON) return 0.0f;
    
    float lumaDiff = abs(currY - histY) / (histY + 0.01f);
    float lumaStability = 1.0f - saturate(lumaDiff * 2.0f);
    
    lumaStability *= 1.0f - saturate(neighborhoodStd * 0.5f);
    
    return saturate(lumaStability);
}

float ComputeNormalConsistency(float3 currentNormal, float3 historyNormal) {
    if (length(currentNormal) < TSS_EPSILON || length(historyNormal) < TSS_EPSILON) {
        return 0.5f;
    }
    
    float dotProd = dot(currentNormal, historyNormal);
    return saturate(dotProd * 0.5f + 0.5f);
}

float PredictHistoryWeight(
    float currentDepth,
    float historyDepth,
    float2 currentMV,
    float2 historyMV,
    float currentLuma,
    float historyLuma,
    float neighborhoodStd,
    float3 currentNormal,
    float3 historyNormal,
    float frameTimeDelta
) {
    float depthTrust = ComputeDepthTrust(currentDepth, historyDepth);
    
    float velocityTrust = ComputeVelocityTrust(currentMV, historyMV);
    
    float lumaStability = ComputeLuminanceStability(currentLuma, historyLuma, neighborhoodStd);
    
    float normalConsistency = ComputeNormalConsistency(currentNormal, historyNormal);
    
    float weight = depthTrust * tss_w_depth +
                   velocityTrust * tss_w_mv +
                   lumaStability * tss_w_luma +
                   normalConsistency * 0.0f;
    
    weight = saturate(weight);
    
    float mvMagnitude = length(currentMV);
    float highVelocityPenalty = step(0.1f, mvMagnitude) * 0.2f;
    weight = max(0.0f, weight - highVelocityPenalty);
    
    float frameTimeBoost = saturate(frameTimeDelta / 0.033f) * 0.1f;
    weight = min(weight + frameTimeBoost, 0.95f);
    
    return weight;
}

float4 NeuralRepair(
    float4 currentColor,
    float4 warpedHistory,
    float disocclusionMask,
    float neuralWeight,
    float velocity,
    float lumaDiff
) {
    float4 result;
    
    if (disocclusionMask > tss_disocclusion_threshold) {
        result = currentColor;
        return result;
    }
    
    float adaptiveWeight = neuralWeight;
    
    float highVelocityBoost = step(0.05f, velocity) * 0.1f;
    adaptiveWeight = min(adaptiveWeight + highVelocityBoost, 0.95f);
    
    float lumaDiffPenalty = saturate(lumaDiff * 2.0f) * 0.2f;
    adaptiveWeight = max(0.0f, adaptiveWeight - lumaDiffPenalty);
    
    if (lumaDiff > 0.3f && velocity < 0.01f) {
        adaptiveWeight = min(adaptiveWeight, 0.1f);
    }
    
    adaptiveWeight = clamp(adaptiveWeight, 0.0f, 0.95f);
    
    result = lerp(currentColor, warpedHistory, adaptiveWeight);
    
    return result;
}

float ComputeDisocclusionMask(
    float currentDepth,
    float historyDepth,
    float2 currentMV,
    float2 historyMV,
    float3 currentNormal,
    float3 historyNormal
) {
    if (currentDepth < TSS_EPSILON) return 1.0f;
    
    float depthDiff = abs(currentDepth - historyDepth) / currentDepth;
    float depthScore = saturate(depthDiff * 10.0f);
    
    float2 mvDiff = currentMV - historyMV;
    float mvDivergence = length(mvDiff);
    float mvScore = saturate(mvDivergence * 2.0f);
    
    float normalDiff = 1.0f - abs(dot(currentNormal, historyNormal));
    float normalScore = normalDiff * 5.0f;
    
    float disocclusionScore = max(max(depthScore, mvScore), normalScore);
    
    return saturate(disocclusionScore);
}

float ComputeJitterScore(
    float3 currentYCC,
    float3 warpedYCC,
    float velocity
) {
    float3 diff = abs(currentYCC - warpedYCC);
    float lumaDiff = abs(diff.x);
    
    float velocityPenalty = step(0.1f, velocity) * 0.3f;
    
    float jitterScore = lumaDiff * 3.0f + velocityPenalty;
    
    return saturate(jitterScore);
}

float ComputeGhostingScore(
    float3 currentYCC,
    float3 warpedYCC,
    float velocity
) {
    float3 diff = abs(currentYCC - warpedYCC);
    float yDiff = abs(diff.x);
    float chromaDiff = (abs(diff.y) + abs(diff.z)) * 0.5f;
    
    float ghostingScore = yDiff * 2.0f + chromaDiff * 0.5f;
    
    float velocityBoost = step(0.5f, velocity) * 0.2f;
    ghostingScore += velocityBoost;
    
    return saturate(ghostingScore);
}

float VarianceClipping(float historyValue, float currentValue, float mean, float stdDev, float kSigma) {
    float minBound = mean - kSigma * stdDev;
    float maxBound = mean + kSigma * stdDev;
    
    if (historyValue < minBound || historyValue > maxBound) {
        float clip = min((maxBound - minBound) / (abs(historyValue - mean) + TSS_EPSILON), 1.0f);
        return lerp(historyValue, currentValue, clip * 0.5f);
    }
    
    return historyValue;
}

float YCoCgVarianceClipping(float3 historyYCC, float3 currentYCC, float3 neighbors[9], float kSigma) {
    float sumY = 0.0f, sumSqY = 0.0f;
    float sumCo = 0.0f, sumSqCo = 0.0f;
    float sumCg = 0.0f, sumSqCg = 0.0f;
    
    for (int i = 0; i < 9; i++) {
        sumY += neighbors[i].x;
        sumSqY += neighbors[i].x * neighbors[i].x;
        sumCo += neighbors[i].y;
        sumSqCo += neighbors[i].y * neighbors[i].y;
        sumCg += neighbors[i].z;
        sumSqCg += neighbors[i].z * neighbors[i].z;
    }
    
    float meanY = sumY / 9.0f;
    float varY = (sumSqY / 9.0f) - (meanY * meanY);
    float stdDevY = sqrt(max(varY, 0.0f));
    
    float meanCo = sumCo / 9.0f;
    float varCo = (sumSqCo / 9.0f) - (meanCo * meanCo);
    float stdDevCo = sqrt(max(varCo, 0.0f));
    
    float meanCg = sumCg / 9.0f;
    float varCg = (sumSqCg / 9.0f) - (meanCg * meanCg);
    float stdDevCg = sqrt(max(varCg, 0.0f));
    
    float clippedY = VarianceClipping(historyYCC.x, currentYCC.x, meanY, stdDevY, kSigma);
    float clippedCo = VarianceClipping(historyYCC.y, currentYCC.y, meanCo, stdDevCo, kSigma * 2.0f);
    float clippedCg = VarianceClipping(historyYCC.z, currentYCC.z, meanCg, stdDevCg, kSigma * 2.0f);
    
    return clippedY;
}

RWTexture2D<float4> g_opticalFlow : register(u0);
RWTexture2D<float4> g_dilatedMotionVectors : register(u1);
RWTexture2D<float> g_dilatedDepth : register(u2);
RWTexture2D<float4> g_reprojectedColor : register(u3);
RWTexture2D<float4> g_denoisedOutput : register(u4);
RWTexture2D<uint> g_lockStatus : register(u5);

Texture2D<float4> g_inputColor : register(t0);
Texture2D<float> g_inputDepth : register(t1);
Texture2D<float2> g_inputMotion : register(t2);
Texture2D<float4> g_historyColor : register(t3);

cbuffer TSSConstants : register(b0)
{
    uint2 g_renderSize;
    uint2 g_displaySize;
    float2 g_jitter;
    float g_deltaTime;
    float2 g_mvScale;
    float g_resetFlag;
    float g_cameraNear;
    float g_cameraFar;
    float g_fovVertical;
    float g_padding;
};

groupshared float4 gs_colorAccum[8][8];
groupshared float gs_depthAccum[8][8];
groupshared uint gs_lockCount[8][8];

float2 RaycastScreenUV(float2 uv, float2 mv)
{
    float depth = g_inputDepth.SampleLevel(g_linearSampler, uv, 0.0f);
    float3 viewPos = ReconstructViewPos(uv, depth);
    float3 worldPos = mul(float4(viewPos, 1.0f), g_viewMatrix).xyz;
    float2 newUV = ProjectToScreen(worldPos + mv);
    return newUV;
}

[numthreads(8, 8, 1)]
void ComputeOpticalFlowCS(uint3 tid : SV_DispatchThreadID)
{
    if (any(tid.xy >= g_renderSize)) return;

    float2 uv = (tid.xy + 0.5f) / g_renderSize;
    float2 pixelSize = 1.0f / g_renderSize;
    
    float4 centerColor = g_inputColor.SampleLevel(g_linearSampler, uv, 0.0f);
    float2 bestOffset = float2(0, 0);
    float bestScore = 1e10f;
    
    float searchRadius = 2.0f * g_deltaTime / 16.67f;
    float2 mvInput = g_inputMotion.SampleLevel(g_linearSampler, uv, 0.0f);
    float2 searchCenter = mvInput * searchRadius * g_deltaTime * 0.001f;
    
    for (int y = -2; y <= 2; y++)
    {
        for (int x = -2; x <= 2; x++)
        {
            float2 offset = float2(x, y) * pixelSize;
            float2 sampleUV = uv + offset;
            
            if (any(sampleUV < 0) || any(sampleUV > 1)) continue;
            
            float4 sampleColor = g_inputColor.SampleLevel(g_linearSampler, sampleUV, 0.0f);
            float3 diff = abs(centerColor.rgb - sampleColor.rgb);
            float sad = dot(diff, 1.0f);
            
            if (sad < bestScore)
            {
                bestScore = sad;
                bestOffset = offset;
            }
        }
    }
    
    g_opticalFlow[tid.xy] = float4(bestOffset * g_renderSize, bestScore, 1.0f);
}

[numthreads(8, 8, 1)]
void DilateMotionVectorsCS(uint3 tid : SV_DispatchThreadID)
{
    if (any(tid.xy >= g_renderSize)) return;

    float2 bestMV = float2(0, 0);
    float bestDepth = 1e10f;
    float2 centerUV = (tid.xy + 0.5f) / g_renderSize;
    
    [unroll]
    for (int y = -3; y <= 3; y++)
    {
        [unroll]
        for (int x = -3; x <= 3; x++)
        {
            int2 samplePos = tid.xy + int2(x, y);
            if (any(samplePos < 0) || any(samplePos >= (int2)g_renderSize)) continue;
            
            float2 mv = g_opticalFlow[samplePos].xy;
            float sampleDepth = g_inputDepth[samplePos];
            
            if (sampleDepth < bestDepth && length(mv) > 0.001f)
            {
                bestDepth = sampleDepth;
                bestMV = mv;
            }
        }
    }
    
    g_dilatedMotionVectors[tid.xy] = float4(bestMV, 0, 1);
    g_dilatedDepth[tid.xy] = g_inputDepth[tid.xy];
}

[numthreads(8, 8, 1)]
void ReprojectFrameCS(uint3 tid : SV_DispatchThreadID)
{
    if (any(tid.xy >= g_displaySize)) return;

    float2 uv = (tid.xy + 0.5f) / g_displaySize;
    float2 halfRenderUV = (floor(uv * g_renderSize * 0.5f) + 0.5f) / (g_renderSize * 0.5f);
    
    float2 mv = g_dilatedMotionVectors[(uint2)(halfRenderUV * g_renderSize)].xy;
    float reprojectUV = g_resetFlag > 0 ? uv : uv - mv * g_deltaTime * 0.001f;
    
    float4 historyColor = g_historyColor.SampleLevel(g_linearSampler, reprojectUV, 0.0f);
    float4 currentColor = g_inputColor.SampleLevel(g_linearSampler, uv, 0.0f);
    
    float weight = g_resetFlag > 0 ? 1.0f : 0.5f;
    float4 reprojected = lerp(historyColor, currentColor, weight);
    
    g_reprojectedColor[tid.xy] = reprojected;
}

[numthreads(8, 8, 1)]
void DenoiseAndOutputCS(uint3 tid : SV_DispatchThreadID)
{
    if (any(tid.xy >= g_displaySize)) return;

    float2 uv = (tid.xy + 0.5f) / g_displaySize;
    float4 centerColor = g_reprojectedColor[tid.xy];
    
    if (g_resetFlag > 0)
    {
        g_denoisedOutput[tid.xy] = g_inputColor.SampleLevel(g_linearSampler, uv, 0.0f);
        return;
    }
    
    float4 colorSum = centerColor;
    float weightSum = 1.0f;
    
    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            if (x == 0 && y == 0) continue;
            
            int2 offset = int2(x, y);
            int2 samplePos = tid.xy + offset;
            
            if (any(samplePos < 0) || any(samplePos >= (int2)g_displaySize)) continue;
            
            float4 sampleColor = g_reprojectedColor[samplePos];
            float depth = g_dilatedDepth[samplePos];
            float centerDepth = g_dilatedDepth[tid.xy];
            
            float depthWeight = 1.0f / (1.0f + 10.0f * abs(depth - centerDepth));
            float temporalWeight = 0.5f;
            
            float weight = depthWeight * temporalWeight;
            colorSum += sampleColor * weight;
            weightSum += weight;
        }
    }
    
    g_denoisedOutput[tid.xy] = colorSum / weightSum;
}

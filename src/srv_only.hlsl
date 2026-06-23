Texture2D<float> src : register(t0);
RWTexture2D<float> dst : register(u4);
[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    dst[tid.xy] = src[uint2(tid.x/2, tid.y/2)];
}

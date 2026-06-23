Texture2D<float> src : register(t0);
RWTexture2D<float> dst : register(u4);
cbuffer C : register(b0) { int w, h, ow, oh; };
[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    if (tid.x >= ow || tid.y >= oh) return;
    // Scale coordinates for 2x upscale
    float sx = tid.x * w / (float)ow;
    float sy = tid.y * h / (float)oh;
    int ix = (int)sx;
    int iy = (int)sy;
    dst[tid.xy] = src[uint2(ix, iy)];
}

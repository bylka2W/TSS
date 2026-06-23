RWTexture2D<float> buf : register(u4);
[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    buf[tid.xy] = (tid.x + tid.y) / 510.0f;
}

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3d12sdklayers.h>
#include <stdio.h>
#include <assert.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#define IW 128
#define IH 128
#define OW 256
#define OH 256

struct TSS_Consts { int w, h, ow, oh; };

template <class T> void SafeRelease(T **p) { if (*p) { (*p)->Release(); *p = nullptr; } }

static const TSS_Consts kConsts = { IW, IH, OW, OH };

static void Check(HRESULT hr) { assert(SUCCEEDED(hr)); }

static void genTestPattern(float *out, UINT w, UINT h) {
    for (UINT y = 0; y < h; y++)
        for (UINT x = 0; x < w; x++) {
            float v = (float)(x + y) / (float)(w + h - 2);
            int bx = x / 8, by = y / 8;
            if ((bx + by) & 1) v = (v > 0.5f) ? 0.1f : 0.9f;
            if (x == w/2 || y == h/2) v = 1.0f;
            out[y * w + x] = v;
        }
}

static void writePPM(const char *path, const float *data, UINT w, UINT h) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", (int)w, (int)h);
    for (UINT y = 0; y < h; y++)
        for (UINT x = 0; x < w; x++) {
            float v = data[y * w + x];
            if (v < 0) v = 0; if (v > 1) v = 1;
            unsigned char c = (unsigned char)(v * 255.0f + 0.5f);
            fwrite(&c, 1, 3, f);
        }
    fclose(f);
    printf("Wrote %s  %dx%d\n", path, (int)w, (int)h);
}

int main() {
    // Debug layer
    ID3D12Debug *dbg = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) {
        dbg->EnableDebugLayer();
        printf("Debug ON\n");
    }

    ID3D12Device *dev = nullptr;
    Check(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&dev)));

    ID3D12CommandQueue *queue = nullptr;
    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    Check(dev->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue)));

    ID3D12CommandAllocator *alloc = nullptr;
    Check(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc)));

    ID3D12GraphicsCommandList *cl = nullptr;
    Check(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc, nullptr, IID_PPV_ARGS(&cl)));

    ID3D12Fence *fence = nullptr;
    Check(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    HANDLE fenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);

    // Root signature
    D3D12_DESCRIPTOR_RANGE srvR = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND };
    D3D12_DESCRIPTOR_RANGE uavR = { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND };
    D3D12_ROOT_PARAMETER rp[3] = {};
    rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rp[0].Descriptor.ShaderRegister = 0;
    rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rp[1].DescriptorTable.NumDescriptorRanges = 1;
    rp[1].DescriptorTable.pDescriptorRanges = &srvR;
    rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rp[2].DescriptorTable.NumDescriptorRanges = 1;
    rp[2].DescriptorTable.pDescriptorRanges = &uavR;
    rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 3;
    rsDesc.pParameters = rp;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ID3DBlob *rsBlob = nullptr, *rsErr = nullptr;
    Check(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &rsErr));
    if (rsErr) { printf("RS warn: %s\n", (const char*)rsErr->GetBufferPointer()); rsErr->Release(); }

    ID3D12RootSignature *rs = nullptr;
    Check(dev->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&rs)));
    rsBlob->Release();

    // PSO
    FILE *cso = fopen("C:\\TSS\\src\\tss_easu.cso", "rb");
    if (!cso) { fprintf(stderr, "FAIL: no .cso\n"); return 1; }
    fseek(cso, 0, SEEK_END);
    UINT csoSize = (UINT)ftell(cso);
    fseek(cso, 0, SEEK_SET);
    void *csoData = malloc(csoSize);
    fread(csoData, 1, csoSize, cso);
    fclose(cso);

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rs;
    psoDesc.CS = { csoData, csoSize };
    ID3D12PipelineState *pso = nullptr;
    Check(dev->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
    free(csoData);

    // Descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC dhd = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
    ID3D12DescriptorHeap *dh = nullptr;
    Check(dev->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&dh)));
    UINT incr = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_CPU_DESCRIPTOR_HANDLE cbvCPU = dh->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE srvCPU = { cbvCPU.ptr + 1 * incr };
    D3D12_CPU_DESCRIPTOR_HANDLE uavCPU = { cbvCPU.ptr + 2 * incr };
    D3D12_GPU_DESCRIPTOR_HANDLE baseGPU = dh->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srvGPU = { baseGPU.ptr + 1 * incr };
    D3D12_GPU_DESCRIPTOR_HANDLE uavGPU = { baseGPU.ptr + 2 * incr };

    // Constant buffer
    D3D12_HEAP_PROPERTIES upHeap = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC bufDesc = { D3D12_RESOURCE_DIMENSION_BUFFER };
    bufDesc.Width = 256;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN; bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource *cbBuf = nullptr;
    Check(dev->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbBuf)));

    TSS_Consts *cbM = nullptr;
    Check(cbBuf->Map(0, nullptr, (void**)&cbM));
    memcpy(cbM, &kConsts, sizeof(kConsts));
    cbBuf->Unmap(0, nullptr);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = cbBuf->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = 256;
    dev->CreateConstantBufferView(&cbvDesc, cbvCPU);

    // Input texture
    D3D12_HEAP_PROPERTIES defHeap = { D3D12_HEAP_TYPE_DEFAULT };
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = IW; texDesc.Height = IH;
    texDesc.DepthOrArraySize = 1; texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R32_FLOAT;
    texDesc.SampleDesc.Count = 1; texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource *inTex = nullptr;
    Check(dev->CreateCommittedResource(&defHeap, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&inTex)));

    // Upload buffer (keep alive until after fence)
    float *pixels = (float*)malloc(IW * IH * sizeof(float));
    genTestPattern(pixels, IW, IH);

    UINT rowPitch = (IW * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    D3D12_RESOURCE_DESC ubDesc = { D3D12_RESOURCE_DIMENSION_BUFFER };
    ubDesc.Width = rowPitch * IH;
    ubDesc.Height = 1; ubDesc.DepthOrArraySize = 1; ubDesc.MipLevels = 1;
    ubDesc.Format = DXGI_FORMAT_UNKNOWN; ubDesc.SampleDesc.Count = 1;
    ubDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; ubDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource *upBuf = nullptr;
    Check(dev->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &ubDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upBuf)));

    void *upMapped = nullptr;
    Check(upBuf->Map(0, nullptr, &upMapped));
    for (UINT y = 0; y < IH; y++)
        memcpy((char*)upMapped + y * rowPitch, pixels + y * IW, IW * sizeof(float));
    upBuf->Unmap(0, nullptr);
    free(pixels);

    D3D12_TEXTURE_COPY_LOCATION srcLoc = { upBuf, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT };
    srcLoc.PlacedFootprint.Offset = 0;
    srcLoc.PlacedFootprint.Footprint.Width = IW;
    srcLoc.PlacedFootprint.Footprint.Height = IH;
    srcLoc.PlacedFootprint.Footprint.Depth = 1;
    srcLoc.PlacedFootprint.Footprint.RowPitch = rowPitch;
    srcLoc.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32_FLOAT;

    D3D12_TEXTURE_COPY_LOCATION dstLoc = { inTex, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX };
    dstLoc.SubresourceIndex = 0;
    cl->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    // SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    dev->CreateShaderResourceView(inTex, &srvDesc, srvCPU);

    // Output UAV texture
    D3D12_RESOURCE_DESC outDesc = {};
    outDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    outDesc.Width = OW; outDesc.Height = OH;
    outDesc.DepthOrArraySize = 1; outDesc.MipLevels = 1;
    outDesc.Format = DXGI_FORMAT_R32_FLOAT;
    outDesc.SampleDesc.Count = 1; outDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    outDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    ID3D12Resource *outTex = nullptr;
    Check(dev->CreateCommittedResource(&defHeap, D3D12_HEAP_FLAG_NONE, &outDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&outTex)));

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    dev->CreateUnorderedAccessView(outTex, nullptr, &uavDesc, uavCPU);

    // Readback buffer
    D3D12_HEAP_PROPERTIES rbHeap = { D3D12_HEAP_TYPE_READBACK };
    D3D12_RESOURCE_DESC rbDesc = { D3D12_RESOURCE_DIMENSION_BUFFER };
    UINT rbRowPitch = (OW * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    rbDesc.Width = rbRowPitch * OH;
    rbDesc.Height = 1; rbDesc.DepthOrArraySize = 1; rbDesc.MipLevels = 1;
    rbDesc.Format = DXGI_FORMAT_UNKNOWN; rbDesc.SampleDesc.Count = 1;
    rbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; rbDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource *rbBuf = nullptr;
    Check(dev->CreateCommittedResource(&rbHeap, D3D12_HEAP_FLAG_NONE, &rbDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&rbBuf)));

    // Record command list
    cl->SetComputeRootSignature(rs);
    cl->SetPipelineState(pso);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = inTex;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cl->ResourceBarrier(1, &barrier);

    ID3D12DescriptorHeap *heaps[] = { dh };
    cl->SetDescriptorHeaps(1, heaps);
    cl->SetComputeRootConstantBufferView(0, cbBuf->GetGPUVirtualAddress());
    cl->SetComputeRootDescriptorTable(1, srvGPU);
    cl->SetComputeRootDescriptorTable(2, uavGPU);
    cl->Dispatch((OW + 7) / 8, (OH + 7) / 8, 1);

    barrier.Transition.pResource = outTex;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    cl->ResourceBarrier(1, &barrier);

    D3D12_TEXTURE_COPY_LOCATION rbs = { outTex, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX };
    rbs.SubresourceIndex = 0;
    D3D12_TEXTURE_COPY_LOCATION rbd = { rbBuf, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT };
    rbd.PlacedFootprint.Offset = 0;
    rbd.PlacedFootprint.Footprint.Width = OW;
    rbd.PlacedFootprint.Footprint.Height = OH;
    rbd.PlacedFootprint.Footprint.Depth = 1;
    rbd.PlacedFootprint.Footprint.RowPitch = rbRowPitch;
    rbd.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32_FLOAT;
    cl->CopyTextureRegion(&rbd, 0, 0, 0, &rbs, nullptr);

    Check(cl->Close());

    // Execute
    ID3D12CommandList *cmds[] = { cl };
    queue->ExecuteCommandLists(1, cmds);
    Check(queue->Signal(fence, 1));
    Check(fence->SetEventOnCompletion(1, fenceEvent));
    WaitForSingleObject(fenceEvent, INFINITE);

    // Readback
    float *rbM = nullptr;
    Check(rbBuf->Map(0, nullptr, (void**)&rbM));
    float *result = (float*)malloc(OW * OH * sizeof(float));
    for (UINT y = 0; y < OH; y++)
        memcpy(result + y * OW, (const char*)rbM + y * rbRowPitch, OW * sizeof(float));
    rbBuf->Unmap(0, nullptr);

    writePPM("C:\\TSS\\src\\dx12_test_output.ppm", result, OW, OH);

    // Write raw float32 for exact comparison
    {
        FILE *rf = fopen("C:\\TSS\\src\\dx12_test_output.f32", "wb");
        if (rf) { fwrite(result, sizeof(float), (size_t)OW * OH, rf); fclose(rf); }
    }

    free(result);

    // Cleanup
    SafeRelease(&upBuf);
    SafeRelease(&outTex);
    SafeRelease(&inTex);
    SafeRelease(&cbBuf);
    SafeRelease(&rbBuf);
    SafeRelease(&dh);
    SafeRelease(&pso);
    SafeRelease(&rs);
    SafeRelease(&cl);
    SafeRelease(&alloc);
    SafeRelease(&queue);
    SafeRelease(&fence);
    SafeRelease(&dev);
    if (dbg) dbg->Release();
    CloseHandle(fenceEvent);

    printf("Done.\n");
    return 0;
}

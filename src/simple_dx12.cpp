#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <stdio.h>
#include <assert.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

template <class T> void SafeRelease(T **p) { if (*p) { (*p)->Release(); *p = nullptr; } }

static void Check(HRESULT hr) { assert(SUCCEEDED(hr)); }

int main() {
    // Enable debug layer
    ID3D12Debug *dbg = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) {
        dbg->EnableDebugLayer();
        printf("Debug layer enabled\n");
    } else {
        printf("No debug layer\n");
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
    HANDLE fe = CreateEventA(nullptr, FALSE, FALSE, nullptr);

    // Root signature (just UAV u4)
    D3D12_DESCRIPTOR_RANGE uavR = { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND };
    D3D12_ROOT_PARAMETER rp = {};
    rp.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rp.DescriptorTable.NumDescriptorRanges = 1;
    rp.DescriptorTable.pDescriptorRanges = &uavR;
    rp.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters = &rp;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ID3DBlob *rsB = nullptr, *rsE = nullptr;
    Check(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsB, &rsE));
    ID3D12RootSignature *rs = nullptr;
    Check(dev->CreateRootSignature(0, rsB->GetBufferPointer(), rsB->GetBufferSize(), IID_PPV_ARGS(&rs)));
    rsB->Release(); if (rsE) rsE->Release();

    // Load minimal .cso
    FILE *f = fopen("C:\\TSS\\src\\simple_test.cso", "rb");
    if (!f) { fprintf(stderr, "FAIL: no simple_test.cso\n"); return 1; }
    fseek(f, 0, SEEK_END); int sz = (int)ftell(f); fseek(f, 0, SEEK_SET);
    void *csod = malloc(sz); fread(csod, 1, sz, f); fclose(f);

    D3D12_COMPUTE_PIPELINE_STATE_DESC psod = {};
    psod.pRootSignature = rs;
    psod.CS = { csod, (size_t)sz };
    ID3D12PipelineState *pso = nullptr;
    Check(dev->CreateComputePipelineState(&psod, IID_PPV_ARGS(&pso)));
    free(csod);

    // Descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC dhd = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
    ID3D12DescriptorHeap *dh = nullptr;
    Check(dev->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&dh)));
    UINT incr = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Output texture 256x256 R32_UAV
    D3D12_HEAP_PROPERTIES def = { D3D12_HEAP_TYPE_DEFAULT };
    D3D12_RESOURCE_DESC td = { D3D12_RESOURCE_DIMENSION_TEXTURE2D };
    td.Width = 256; td.Height = 256; td.DepthOrArraySize = 1; td.MipLevels = 1;
    td.Format = DXGI_FORMAT_R32_FLOAT;
    td.SampleDesc.Count = 1; td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    td.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ID3D12Resource *tex = nullptr;
    Check(dev->CreateCommittedResource(&def, D3D12_HEAP_FLAG_NONE, &td, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&tex)));

    D3D12_UNORDERED_ACCESS_VIEW_DESC uvd = {};
    uvd.Format = DXGI_FORMAT_R32_FLOAT;
    uvd.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    dev->CreateUnorderedAccessView(tex, nullptr, &uvd, dh->GetCPUDescriptorHandleForHeapStart());

    // Readback
    D3D12_HEAP_PROPERTIES rbhp = { D3D12_HEAP_TYPE_READBACK };
    D3D12_RESOURCE_DESC rbd = { D3D12_RESOURCE_DIMENSION_BUFFER };
    rbd.Width = 256 * 256 * 4; rbd.Height = 1; rbd.DepthOrArraySize = 1; rbd.MipLevels = 1;
    rbd.Format = DXGI_FORMAT_UNKNOWN; rbd.SampleDesc.Count = 1;
    rbd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; rbd.Flags = D3D12_RESOURCE_FLAG_NONE;
    ID3D12Resource *rb = nullptr;
    Check(dev->CreateCommittedResource(&rbhp, D3D12_HEAP_FLAG_NONE, &rbd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&rb)));

    // Record & dispatch
    cl->SetComputeRootSignature(rs);
    cl->SetPipelineState(pso);
    ID3D12DescriptorHeap *heaps[] = { dh };
    cl->SetDescriptorHeaps(1, heaps);
    cl->SetComputeRootDescriptorTable(0, dh->GetGPUDescriptorHandleForHeapStart());
    cl->Dispatch(32, 32, 1);

    // Copy to readback
    D3D12_RESOURCE_BARRIER b = {};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = tex;
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cl->ResourceBarrier(1, &b);

    D3D12_TEXTURE_COPY_LOCATION src = { tex, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX };
    src.SubresourceIndex = 0;
    D3D12_TEXTURE_COPY_LOCATION dst = { rb, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT };
    dst.PlacedFootprint.Offset = 0;
    dst.PlacedFootprint.Footprint.Width = 256;
    dst.PlacedFootprint.Footprint.Height = 256;
    dst.PlacedFootprint.Footprint.Depth = 1;
    dst.PlacedFootprint.Footprint.RowPitch = 1024;
    dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32_FLOAT;
    cl->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    Check(cl->Close());

    ID3D12CommandList *cml[] = { cl };
    queue->ExecuteCommandLists(1, cml);
    Check(queue->Signal(fence, 1));
    Check(fence->SetEventOnCompletion(1, fe));
    WaitForSingleObject(fe, INFINITE);

    // Readback
    float *m = nullptr;
    Check(rb->Map(0, nullptr, (void**)&m));
    int sum = 0, nz = 0; float mx = 0;
    for (int i = 0; i < 256 * 256; i++) {
        if (m[i] != 0) { sum += (int)(m[i] * 255); nz++; if (m[i] > mx) mx = m[i]; }
    }
    printf("NonZeroPixels=%d Avg=%.2f Max=%.4f\n", nz, nz ? sum / (float)nz : 0, mx);

    // Write PPM
    FILE *ppm = fopen("C:\\TSS\\src\\simple_test.ppm", "wb");
    fprintf(ppm, "P6\n256 256\n255\n");
    for (int i = 0; i < 256 * 256; i++) {
        float v = m[i]; if (v > 1) v = 1; if (v < 0) v = 0;
        unsigned char c = (unsigned char)(v * 255 + 0.5f);
        fwrite(&c, 1, 3, ppm);
    }
    fclose(ppm);
    printf("Wrote simple_test.ppm\n");

    rb->Unmap(0, nullptr);
    SafeRelease(&tex); SafeRelease(&rb); SafeRelease(&dh);
    SafeRelease(&pso); SafeRelease(&rs); SafeRelease(&cl);
    SafeRelease(&alloc); SafeRelease(&queue); SafeRelease(&fence);
    SafeRelease(&dev); if (dbg) dbg->Release();
    CloseHandle(fe);
    return 0;
}

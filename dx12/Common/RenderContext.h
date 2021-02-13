#pragma once

#include <wrl.h>
using namespace Microsoft::WRL;

struct ID3D12Device2;
struct IDXGIFactory4;
struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;

class RenderContext
{
public:
    static RenderContext& Get();
    void Init();
    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;
    ID3D12Device2* Device();
    ID3D12CommandQueue* CommandQueue();
    ID3D12GraphicsCommandList* CommandList();
    ID3D12CommandAllocator* CommandAllocator();
    IDXGIFactory4* GetDXGIFactory();
    inline bool IsTearingAvailable() { return bAllowTearing; }
private:
    RenderContext() = default;
    ~RenderContext() = default;

private:
    ComPtr<ID3D12Device2> mDevice;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<IDXGIFactory4> mDXGIFactory4;
    bool bAllowTearing;
};


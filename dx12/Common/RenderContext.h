#pragma once

#include <wrl.h>
using namespace Microsoft::WRL;

struct ID3D12Device2;
struct IDXGIFactory4;

class RenderContext
{
public:
    static RenderContext& Get();
    void Init();
    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;
    ID3D12Device2* GetDevice();
    IDXGIFactory4* GetDXGIFactory();
    inline bool IsTearingAvailable() { return bAllowTearing; }
private:
    RenderContext() = default;
    ~RenderContext() = default;

private:
    ComPtr<ID3D12Device2> mDevice;
    ComPtr<IDXGIFactory4> mDXGIFactory4;
    bool bAllowTearing;
};


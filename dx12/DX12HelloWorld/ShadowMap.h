#pragma once

#include "../Common/MathHelper.h"
#include "../Common/d3dx12.h"

class ID3D12Device;
class ID3D12Resource;

class ShadowMap
{
public:
    ShadowMap(ID3D12Device* device, UINT width, UINT height);
    ShadowMap(const ShadowMap& rhs) = delete;
    ShadowMap& operator=(const ShadowMap& rhs) = delete;
    ~ShadowMap() = default;

    UINT Width() const;
    UINT Height() const;
    ID3D12Resource* Resource();
    CD3DX12_GPU_DESCRIPTOR_HANDLE Srv() const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE Dsv() const;

    D3D12_VIEWPORT Viewport() const;
    D3D12_RECT ScissorRect() const;

    void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
                          CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
                          CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);

private:
    void BuildDescriptors();
    void BuildResource();

private:
    ID3D12Device* mDevice;
    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;
    UINT mWidth;
    UINT mHeight;
    DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mHCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mHGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mHCpuDsv;

    Microsoft::WRL::ComPtr<ID3D12Resource> mShadowMap = nullptr;
};


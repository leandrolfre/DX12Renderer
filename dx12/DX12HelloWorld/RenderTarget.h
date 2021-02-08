#pragma once

#include<memory>
#include "../Common/d3dx12.h"

struct ID3D12Resource;
struct ID3D12Device;

class RenderTarget
{
public:
	RenderTarget(UINT64 rtWidth, UINT64 rtHeight, DXGI_FORMAT rtFormat);
	~RenderTarget() = default;
	ID3D12Resource* Resource() { return mResource.Get(); }
public:
	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuRtv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GpuSrv;
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
	UINT64 mWidth;
	UINT64 mHeight;
	DXGI_FORMAT mFormat;
};
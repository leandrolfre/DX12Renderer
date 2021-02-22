#pragma once

#include <wrl.h>
#include <d3d12.h>
#include "../Common/util.h"
#include "../Common/d3dx12.h"

using namespace Microsoft::WRL;

class DescriptorHeap
{
public:
	DescriptorHeap(ID3D12Device* device, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag);
	~DescriptorHeap() = default;
	DescriptorHeap(const DescriptorHeap&) = delete;
	DescriptorHeap& operator=(const DescriptorHeap&) = delete;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle();
	ID3D12DescriptorHeap* Heap();
	UINT HeapSize();

private:
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	UINT mDescriptorHeapSize;
	UINT mCPUHandleIndex = 0;
	UINT mGPUHandleIndex = 0;
};

class ResourceManager
{
public:
	static ResourceManager& Get();
	CD3DX12_CPU_DESCRIPTOR_HANDLE AllocShaderResource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE AllocGPUShaderResource();
	CD3DX12_CPU_DESCRIPTOR_HANDLE AllocRenderTargetResource();
	CD3DX12_CPU_DESCRIPTOR_HANDLE AllocDepthStencilResource();
	ID3D12DescriptorHeap* DescriptorHeaps();
	ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer, ID3D12GraphicsCommandList* cmdList);
	void CreateDefaultBuffer(const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& resource, ComPtr<ID3D12Resource>& uploadBuffer, ID3D12GraphicsCommandList* cmdList);
	ComPtr<ID3DBlob> LoadBinary(const std::string& filename);
private:
	ResourceManager();
	~ResourceManager() = default;
	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;
private:
	std::unique_ptr<DescriptorHeap> mShaderDescriptorHeap;
	std::unique_ptr<DescriptorHeap> mRenderTargetDescriptorHeap;
	std::unique_ptr<DescriptorHeap> mDepthDescriptorHeap;
};


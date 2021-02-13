#include "ResourceManager.h"
#include "../Common/RenderContext.h"

DescriptorHeap::DescriptorHeap(ID3D12Device* device, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
    heapDesc.Type = type;
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Flags = flag;
    heapDesc.NodeMask = 0;
    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mDescriptorHeap)));

    mDescriptorHeapSize = device->GetDescriptorHandleIncrementSize(type);
}

ID3D12DescriptorHeap* DescriptorHeap::Heap()
{
    return mDescriptorHeap.Get();
}

UINT DescriptorHeap::HeapSize()
{
    return mDescriptorHeapSize;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::CpuHandle()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), mCPUHandleIndex++, mDescriptorHeapSize);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GpuHandle()
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), mGPUHandleIndex++, mDescriptorHeapSize);
}

ResourceManager::ResourceManager()
{
    auto Device = RenderContext::Get().Device();
    int heapSize = 100;
    mShaderDescriptorHeap = std::make_unique<DescriptorHeap>(Device, heapSize, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    mRenderTargetDescriptorHeap = std::make_unique<DescriptorHeap>(Device, heapSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    mDepthDescriptorHeap = std::make_unique<DescriptorHeap>(Device, heapSize, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    mShaderDescriptorHeap->Heap()->SetName(L"SRV Heap");
    mRenderTargetDescriptorHeap->Heap()->SetName(L"RT Heap");
    mDepthDescriptorHeap->Heap()->SetName(L"DSV Heap");
}

ResourceManager& ResourceManager::Get()
{
    static ResourceManager instance;
    return instance;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ResourceManager::AllocShaderResource()
{
    return mShaderDescriptorHeap->CpuHandle();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ResourceManager::AllocGPUShaderResource()
{
    return mShaderDescriptorHeap->GpuHandle();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ResourceManager::AllocRenderTargetResource()
{
    return mRenderTargetDescriptorHeap->CpuHandle();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ResourceManager::AllocDepthStencilResource()
{
    return mDepthDescriptorHeap->CpuHandle();
}

ID3D12DescriptorHeap* ResourceManager::DescriptorHeaps()
{
    return mShaderDescriptorHeap->Heap();
}

ComPtr<ID3D12Resource> ResourceManager::CreateDefaultBuffer(const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer, ID3D12GraphicsCommandList* cmdList)
{
    auto Device = RenderContext::Get().Device();
    ComPtr<ID3D12Resource> defaultBuffer;

    ThrowIfFailed(Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())
    ));

    ThrowIfFailed(Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())
    ));

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    return defaultBuffer;   
}


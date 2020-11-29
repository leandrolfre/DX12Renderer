#include "FrameResource.h"
#include "..\Common\Util.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT64 rtWidth, UINT64 rtHeight, DXGI_FORMAT rtFormat, ShaderResourceBuffer* srb, ShaderResourceBuffer* rtb) : Fence(0)
{
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdListAllocator.GetAddressOf())));
    PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
    MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, false);

    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));

    texDesc.Width = rtWidth;
    texDesc.Height = rtHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = rtFormat;
    texDesc.Alignment = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&ScreenMap)
    ));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = rtFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    ScreenCpuSrv = srb->CpuHandle();
    ScreenGpuSrv = srb->GpuHandle();
    device->CreateShaderResourceView(ScreenMap.Get(), &srvDesc, ScreenCpuSrv);
    ScreenCpuRtv = rtb->CpuHandle();
    device->CreateRenderTargetView(ScreenMap.Get(), nullptr, ScreenCpuRtv);
}

ShaderResourceBuffer::ShaderResourceBuffer(ID3D12Device* device, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag)
{
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
    srvHeapDesc.Type = type;
    srvHeapDesc.NumDescriptors = numDescriptors;
    srvHeapDesc.Flags = flag;
    srvHeapDesc.NodeMask = 0;
    ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

    mCbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(type);
}

ID3D12DescriptorHeap* ShaderResourceBuffer::DescriptorHeap()
{
    return mSrvDescriptorHeap.Get();
}

UINT ShaderResourceBuffer::DescriptorHeapSize()
{
    return mCbvSrvUavDescriptorSize;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShaderResourceBuffer::CpuHandle()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), curSrvCPUHandleIndex++, mCbvSrvUavDescriptorSize);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShaderResourceBuffer::GpuHandle()
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), curSrvGPUHandleIndex++, mCbvSrvUavDescriptorSize);
}

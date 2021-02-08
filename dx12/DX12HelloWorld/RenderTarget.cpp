#include "RenderTarget.h"
#include "d3d12.h"
#include "../Common/Util.h"
#include "../Common/RenderContext.h"
#include "ResourceManager.h"

RenderTarget::RenderTarget(UINT64 width, UINT64 height, DXGI_FORMAT format) : mWidth(width), mHeight(height), mFormat(format)
{
    auto Device = RenderContext::Get().GetDevice();
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));

    texDesc.Width = mWidth;
    texDesc.Height = mHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = mFormat;
    texDesc.Alignment = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mFormat;
    optClear.Color[0] = 0.0f;
    optClear.Color[1] = 0.0f;
    optClear.Color[2] = 0.0f;
    optClear.Color[3] = 1.0f;

    ThrowIfFailed(Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&mResource)
    ));
    
    CpuSrv = ResourceManager::Get().AllocShaderResource();
    GpuSrv = ResourceManager::Get().AllocGPUShaderResource();
    CpuRtv = ResourceManager::Get().AllocRenderTargetResource();

    D3D12_RENDER_TARGET_VIEW_DESC rtDesc;
    ZeroMemory(&rtDesc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));
    rtDesc.Format = mFormat;
    rtDesc.Texture2D.MipSlice = 0;
    rtDesc.Texture2D.PlaneSlice = 0;
    rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    Device->CreateRenderTargetView(mResource.Get(), &rtDesc, CpuRtv);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

    srvDesc.Format = mFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    Device->CreateShaderResourceView(mResource.Get(), &srvDesc, CpuSrv);
}



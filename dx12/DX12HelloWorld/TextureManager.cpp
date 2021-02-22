#include "TextureManager.h"
#include "../Common/DDSTextureLoader.h"
#include "../Common/RenderContext.h"
#include "ResourceManager.h"
#include "../Common/Util.h"
#include <dxgiformat.h>

TextureManager& TextureManager::Get()
{
    static TextureManager instance;
    return instance;
}

bool TextureManager::LoadTexture(const std::wstring& path)
{
    auto device = RenderContext::Get().Device();
    auto cmdList = RenderContext::Get().CommandList();
    auto texture = std::make_unique<Texture>();
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device, cmdList, path.c_str(), texture->Resource, texture->UploadHeap));
    mTextureMap.insert({ path, std::move(texture) });
    return false;
}

Texture* TextureManager::getTexture(const std::wstring& path)
{
    auto textureIt = mTextureMap.find(path);
    if (textureIt == mTextureMap.end())
    {
        LoadTexture(path);
    }

    return mTextureMap[path].get();
}

Texture* TextureManager::CreateTexture(const std::wstring& Name, UINT Width, UINT Height, DXGI_FORMAT Format, const void* Data, UINT BPP)
{
    auto texture = std::make_unique<Texture>();
    
    auto Device = RenderContext::Get().Device();
    auto CmdList = RenderContext::Get().CommandList();
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));

    texDesc.Width = Width;
    texDesc.Height = Height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = Format;
    texDesc.Alignment = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc, 
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(texture->Resource.GetAddressOf())
    ));

    const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->Resource.Get(), 0, num2DSubresources);

    ThrowIfFailed(Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(texture->UploadHeap.GetAddressOf())
    ));

    auto rowBytes = (Width * BPP + 7) / 8; // round up to nearest byte
    auto numBytes = rowBytes * Height;
    
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = Data;
    subResourceData.RowPitch = rowBytes;
    subResourceData.SlicePitch = numBytes;

    UpdateSubresources<1>(CmdList, texture->Resource.Get(), texture->UploadHeap.Get(), 0, 0, 1, &subResourceData);

    CmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    mTextureMap.insert({ Name, std::move(texture) });

    return mTextureMap[Name].get();
}
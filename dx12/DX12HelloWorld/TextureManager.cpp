#include "TextureManager.h"
#include "../Common/DDSTextureLoader.h"
#include "../Common/RenderContext.h"
#include "../Common/Util.h"

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


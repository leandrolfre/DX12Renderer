#include "TextureManager.h"
#include "../Common/DDSTextureLoader.h"
#include "Texture.h"

TextureManager& TextureManager::Get()
{
    static TextureManager instance;
    return instance;
}

bool TextureManager::LoadTexture(const std::wstring& path, const std::string& name)
{
    mTextureMap.insert({ name , std::make_unique<Texture>() });
    //ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mDevice.Get(), mCommandList.Get(), path.c_str(), mTextures[i]->Resource, mTextures[i]->UploadHeap));

    return false;
}

Texture* TextureManager::getTexture(const std::string& name)
{
    auto textureIt = mTextureMap.find(name);
    if (textureIt != mTextureMap.end())
    {
        return mTextureMap[name].get();
    }
    return nullptr;
}


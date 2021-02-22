#pragma once

#include <unordered_map>
#include <memory>
#include "Texture.h"

enum DXGI_FORMAT;

class TextureManager
{
public:
    static TextureManager& Get();
    Texture* getTexture(const std::wstring& path);
    Texture* CreateTexture(const std::wstring& Name, UINT Width, UINT Height, DXGI_FORMAT Format, const void* Data, UINT BPP);
private:
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    void operator=(const TextureManager&) = delete;
    bool LoadTexture(const std::wstring& path);
private:
    std::unordered_map<std::wstring, std::unique_ptr<Texture>> mTextureMap;
};


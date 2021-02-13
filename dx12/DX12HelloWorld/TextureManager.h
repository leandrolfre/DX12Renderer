#pragma once

#include <unordered_map>
#include <memory>
#include "Texture.h"

class TextureManager
{
public:
    static TextureManager& Get();
    Texture* getTexture(const std::wstring& path);
private:
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    void operator=(const TextureManager&) = delete;
    bool LoadTexture(const std::wstring& path);
private:
    std::unordered_map<std::wstring, std::unique_ptr<Texture>> mTextureMap;
};


#pragma once

#include <unordered_map>
#include <memory>

class Texture;

class TextureManager
{
public:
    static TextureManager& Get();
    bool LoadTexture(const std::wstring& path, const std::string& name);
    Texture* getTexture(const std::string& name);
private:
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    void operator=(const TextureManager&) = delete;
private:
    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextureMap;
};


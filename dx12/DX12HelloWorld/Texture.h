#pragma once

#include <wrl.h>

struct ID3D12Resource;

struct Texture
{
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap;
};


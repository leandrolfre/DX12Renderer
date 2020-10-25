#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <string>
#include <unordered_map>

#include "../Common/MathHelper.h"

using namespace Microsoft::WRL;

extern const int gNumFrameResources;

struct Material;

struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;
	DirectX::BoundingBox bounds;
};

struct MeshGeometry
{
	std::string Name;
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;
	ComPtr<ID3DBlob> VertexBufferCPU;
	ComPtr<ID3DBlob> IndexBufferCPU;
	ComPtr<ID3D12Resource> VertexBufferGPU;
	ComPtr<ID3D12Resource> IndexBufferGPU;
	ComPtr<ID3D12Resource> VertexBufferUploader;
	ComPtr<ID3D12Resource> IndexBufferUploader;

	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	UINT IndexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vbv.SizeInBytes = VertexBufferByteSize;
		vbv.StrideInBytes = VertexByteStride;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.SizeInBytes = IndexBufferByteSize;
		ibv.Format = IndexFormat;

		return ibv;
	}

	void DisposeUploaders()
	{
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};

class RenderItem
{
public:
    RenderItem() = default;

public:
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	MeshGeometry* Geo = nullptr;
	Material* Mat = nullptr;
    int NumFramesDirty = gNumFrameResources;
	int BaseVertexLocation = 0;
    UINT ObjCBIndex = 0;
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};


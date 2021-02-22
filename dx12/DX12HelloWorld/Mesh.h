#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <string>
#include <vector>
#include "FrameResource.h"

#include "../Common/MathHelper.h"

using namespace Microsoft::WRL;
using namespace DirectX;

struct Vertex
{
	XMFLOAT3 Pos; //12
	XMFLOAT3 Tangent; //12
	XMFLOAT3 Normal; //12
	XMFLOAT2 Tex0; //8
	XMFLOAT2 Tex1; //8
	XMFLOAT4 Color; //16
};

struct MeshData;

class Mesh
{
public:
	Mesh(MeshData& meshData, Material* material);
	~Mesh() = default;
	void Draw(ID3D12GraphicsCommandList* cmdList, UploadBuffer<ObjectConstants>* objectCB = nullptr);
	void UploadData(ID3D12GraphicsCommandList* cmdList);

public:
	XMFLOAT4X4 World = MathHelper::Identity4x4();

private:
	struct SubmeshGeometry
	{
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		INT BaseVertexLocation = 0;
		DirectX::BoundingBox Bounds;
		Material* Mat;
		UINT ObjCBIndex = 0;
	};
	std::vector<SubmeshGeometry> mSubmeshes;
	std::vector<Vertex> mVertices;
	std::vector<UINT16> mIndices;
	ComPtr<ID3DBlob> mVertexBufferCPU;
	ComPtr<ID3DBlob> mIndexBufferCPU;
	ComPtr<ID3D12Resource> mVertexBufferGPU;
	ComPtr<ID3D12Resource> mIndexBufferGPU;
	ComPtr<ID3D12Resource> mVertexBufferUploader;
	ComPtr<ID3D12Resource> mIndexBufferUploader;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
	UINT mVertexByteStride = 0;
	UINT mVertexBufferByteSize = 0;
	UINT mIndexBufferByteSize = 0;
	DXGI_FORMAT mIndexFormat = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
private:
	void DisposeUploaders();
};
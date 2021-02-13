#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <memory>

#include "RenderTarget.h"
#include "../Common/util.h"
#include "../Common/d3dx12.h"
#include "../Common/MathHelper.h"

struct Material
{
	std::string Name;
	UINT MatCBIndex = 0;
	UINT DiffuseSrvHeapIndex = 0;
	int NumFramesDirty;
	int hasNormalMap = 0;
	float Roughness = 0.25f;
	float pad1;
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	CD3DX12_GPU_DESCRIPTOR_HANDLE TextureHandle;
};

struct Light
{
	DirectX::XMFLOAT3 Strength;
	float FalloffStart;
	DirectX::XMFLOAT3 Direction;
	float FalloffEnd;
	DirectX::XMFLOAT3 Position;
	float SpotPower;
	DirectX::XMFLOAT4X4 ShadowViewProj;
	DirectX::XMFLOAT4X4 ShadowTransform;
};

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World;
	UINT MaterialIndex;
	UINT     ObjPad0;
	UINT     ObjPad1;
	UINT     ObjPad2;
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;
	DirectX::XMFLOAT4 AmbientLight;
	Light Lights[16];
};

struct MaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo;
	DirectX::XMFLOAT3 FresnelR0;
	float Roughness;
	UINT DiffuseMapIndex;
	UINT MaterialPad0;
	UINT MaterialPad1;
	int hasNormalMap;
};

template<class T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isContantBuffer) : mIsConstantBuffer(isContantBuffer), mElementCount(elementCount), mCurrElement(0)
	{
		mElementByteSize = isContantBuffer ? (sizeof(T) + 255) & ~255 : sizeof(T);

		ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
													  D3D12_HEAP_FLAG_NONE,
													  &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
													  D3D12_RESOURCE_STATE_GENERIC_READ,
													  nullptr,
													  IID_PPV_ARGS(&mUploadBuffer)));

		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));


	}

	~UploadBuffer()
	{
		mUploadBuffer->Unmap(0, nullptr);
	}

	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
	}

	D3D12_GPU_VIRTUAL_ADDRESS CopyData(const T& data)
	{
		assert(mCurrElement <= mElementCount);
		CopyData(mCurrElement, data);
		auto address = mUploadBuffer.Get()->GetGPUVirtualAddress() + (mCurrElement * mElementByteSize);
		mCurrElement++;
		return address;
	}

	void Reset()
	{
		mCurrElement = 0;
	}
	
	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	
	inline ID3D12Resource* Resource() const
	{
		return mUploadBuffer.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData;
	UINT mElementByteSize;
	UINT mElementCount;
	UINT mCurrElement;
	bool mIsConstantBuffer;
};

enum class GBuffer
{
	Position,
	/*Normal,
	Albedo,
	Specular,*/
	NumGBuffer
};

class FrameResource
{
public:
	FrameResource(UINT passCount, UINT64 rtWidth, UINT64 rtHeight, DXGI_FORMAT rtFormat);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource()
	{};

public:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAllocator;
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB;
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB;
	std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB;
	std::unique_ptr<RenderTarget> RT[2];
	std::unique_ptr<RenderTarget> GBuffer[int(GBuffer::NumGBuffer)];
	DXGI_FORMAT GBufferFormat[int(GBuffer::NumGBuffer)] = { DXGI_FORMAT_R16G16B16A16_FLOAT };// , DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM};
	UINT64 Fence;
};
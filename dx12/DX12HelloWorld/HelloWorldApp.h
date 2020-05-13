#pragma once

#include "../Common/D3DApp.h"

using namespace DirectX;

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj;
};

class D3DAppHelloWorld : public D3DApp
{
public:
	D3DAppHelloWorld(HINSTANCE hInstance) : D3DApp(hInstance)
	{};
	~D3DAppHelloWorld()
	{};
	D3DAppHelloWorld(const D3DAppHelloWorld& rhs) = delete;
	D3DAppHelloWorld& operator=(const D3DAppHelloWorld& rhs) = delete;
	bool Initialize(WNDPROC proc) override;

protected:
	void Update() override;
	void Render() override;

private:
	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

private:
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();;
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap;
	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;
	ComPtr<ID3D12PipelineState> mPSO;
	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB;
	std::unique_ptr<MeshGeometry> mBoxGeo;
};
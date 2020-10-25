#pragma once

#include "../Common/D3DApp.h"
#include "FrameResource.h"
#include "RenderItem.h"
#include "BlurFilter.h"
#include <unordered_map>
#include "Camera.h"

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

enum class RenderLayer
{
	Opaque,
	Transparent,
	Sky,
	RenderLayerCount
};

class D3DAppHelloWorld : public D3DApp
{
public:
	D3DAppHelloWorld(HINSTANCE hInstance) : D3DApp(hInstance)
	{};
	~D3DAppHelloWorld() 
	{
		if (mDevice != nullptr)
			FlushCommandQueue();
	};
	D3DAppHelloWorld(const D3DAppHelloWorld& rhs) = delete;
	D3DAppHelloWorld& operator=(const D3DAppHelloWorld& rhs) = delete;
	bool Initialize(WNDPROC proc) override;
	LRESULT	MsgProc(HWND hwnd, UINT	msg, WPARAM wParam, LPARAM	lParam) override;

protected:
	void Update() override;
	void Render() override;
	void OnMouseMove(WPARAM	btnState, int x, int y) override;

private:
	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPSO();
	void BuildFrameResources();
	void UpdateObjectCBs();
	void UpdateMainPassCB();
	void UpdateMaterialCB();
	void BuildShapeGeometry();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems);
	void BuildMaterials();
	void BuildTextures();
	void OnKeyboardInput();

private:
	PassConstants mMainPassCB;
	Camera mCam;
	XMFLOAT2 mLastMousePos = {0.0f, 0.0f};
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	std::vector<std::unique_ptr<RenderItem>> mAllItems;
	std::vector<RenderItem*> mLayerRItems[(int)RenderLayer::RenderLayerCount];
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map < std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12RootSignature> mPostProcessRootSignature;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap;
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> mUISrvDescriptorHeap;
	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;
	ComPtr<ID3DBlob> mcsHorizontalByteCode;
	ComPtr<ID3DBlob> mcsVerticalByteCode;
	ComPtr<ID3DBlob> mSkyVsByteCode;
	ComPtr<ID3DBlob> mSkyPsByteCode;
	std::unique_ptr<BlurFilter> mBlurFilter;
	FrameResource* mCurrentFrameResource = nullptr;
	UINT mCurrentFrameResourceIndex = 0;
	int mPassCbvOffset = 0;
	bool mIsWireframe = false;
	bool mIsBlurEnabled = false;
	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;
};
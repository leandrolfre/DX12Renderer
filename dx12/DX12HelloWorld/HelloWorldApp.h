#pragma once

#include "../Common/D3DApp.h"
#include "FrameResource.h"
#include "BlurFilter.h"
#include <unordered_map>
#include "Camera.h"
#include "Mesh.h"
#include "ShadowMap.h"
#include "SSAO.h"

using namespace DirectX;

enum class RenderLayer
{
	Opaque,
	Transparent,
	Sky,
	Fullscreen,
	RenderLayerCount
};

class D3DAppHelloWorld : public D3DApp
{
public:
	D3DAppHelloWorld(HINSTANCE hInstance) : D3DApp(hInstance)
	{};
	~D3DAppHelloWorld() 
	{
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
	void BuildRootSignature();
	void BuildDefaultRootSignature();
	void BuildBlurRootSignature();
	void BuildScreenRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPSO();
	void BuildFrameResources();
	void UpdateMainPassCB();
	void UpdateMaterialCB();
	void BuildScene();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<std::unique_ptr<Mesh>>& rItems);
	void BuildMaterials();
	void OnKeyboardInput();
	void BasePass();
	void CompositePass();
	void RenderSky();
	void ShadowPass();
	void BeginRender();
	void EndRender();

private:
	PassConstants mMainPassCB;
	Camera mCam;
	XMFLOAT2 mLastMousePos = {0.0f, 0.0f};
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	std::vector<std::unique_ptr<Mesh>> mLayerRItems[(int)RenderLayer::RenderLayerCount];
	std::unordered_map < std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12RootSignature> mPostProcessRootSignature;
	ComPtr<ID3D12RootSignature> mScreenRootSignature;
	ComPtr<ID3D12DescriptorHeap> mUISrvDescriptorHeap;
	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;
	ComPtr<ID3DBlob> mcsHorizontalByteCode;
	ComPtr<ID3DBlob> mcsVerticalByteCode;
	ComPtr<ID3DBlob> mSkyVsByteCode;
	ComPtr<ID3DBlob> mSkyPsByteCode;
	ComPtr<ID3DBlob> mQuadVsByteCode;
	ComPtr<ID3DBlob> mQuadPsByteCode;
	ComPtr<ID3DBlob> mShadowVSByteCode;
	std::unique_ptr<ShadowMap> mShadowMap;
	std::unique_ptr<SSAO> mSSAO;
	FrameResource* mCurrentFrameResource = nullptr;
	Mesh* mLightBox;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mCubeMapHandle;
	BoundingSphere mSceneBounds;
	UINT mCurrentFrameResourceIndex = 0;
	int mPassCbvOffset = 0;
	bool mIsWireframe = false;
	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;
};
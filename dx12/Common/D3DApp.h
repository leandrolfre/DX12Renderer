#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <d3dcompiler.h>

#include <dxgi1_6.h>

#if defined(DEBUG) || defined(_DEBUG)
#include <d3d12sdklayers.h>
#endif

// D3D12 extension library.
#include "d3dx12.h"
#include "MathHelper.h"

#include <string>
#include <unordered_map>
#include "Util.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

struct ObjectConstants;

class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	virtual ~D3DApp();
    D3DApp(const D3DApp& rhs) = delete;
    D3DApp& operator=(const D3DApp& rhs) = delete;

public:
	int	Run();
	virtual bool Initialize(WNDPROC proc);
	virtual	LRESULT	MsgProc(HWND hwnd, UINT	msg, WPARAM wParam, LPARAM	lParam);

protected:		
	virtual	void CreateRtvAndDsvDescriptorHeaps(); 			
	virtual	void Update() = 0;
	virtual	void Render() = 0;

	//Convenience overrides	for	handling	mouse	input.		
	virtual	void OnMouseDown(WPARAM	btnState, int x, int y){}		
	virtual	void OnMouseUp(WPARAM btnState, int x, int y){}
	virtual	void OnMouseMove(WPARAM	btnState, int x, int y){}

protected:
	bool InitMainWindow(WNDPROC proc);
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();
	ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);
	ComPtr<ID3DBlob> LoadBinary(const std::string& filename);
	float AspectRatio()  const;
	void FlushCommandQueue();

protected:
	HINSTANCE mhInstance;
	HWND mhMainWnd;
	static const UINT mSwapChainBufferCount = 2;
	ComPtr<IDXGIFactory4> mDXGIFactory4;
	ComPtr<ID3D12Device2> mDevice;
	ComPtr<ID3D12Fence> mFence;
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<IDXGISwapChain4> mSwapChain;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	ComPtr<ID3D12Resource> mSwapChainBuffers[mSwapChainBufferCount];
	ComPtr<ID3D12Resource> mDepthStencilBuffer;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	UINT64 mFenceValues[mSwapChainBufferCount];
	UINT64 mFenceValue;
	HANDLE mFenceEvent;
	UINT mClientWidth;
	UINT mClientHeight;
	UINT mCurrentBufferIndex;
	UINT mDsvDescriptorSize;
	UINT mRtvDescriptorSize;
	UINT mCbvSrvUavDescriptorSize;
	const wchar_t* mWindowClassName;
	const wchar_t* mWindowTitleName;
	bool mAllowTearing;

};


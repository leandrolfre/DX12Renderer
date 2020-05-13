#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>

#include <dxgi1_6.h>
#include <assert.h>
#include <exception>
#include <comdef.h>

#if defined(DEBUG) || defined(_DEBUG)
#include <d3d12sdklayers.h>
#endif

// D3D12 extension library.
#include "d3dx12.h"
#include "MathHelper.h"

#include <string>
#include <unordered_map>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

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
	void FlushCommandQueue();
	ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);
	ComPtr<ID3DBlob> LoadBinary(const std::string& filename);
	float AspectRatio()  const;

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

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNumber) : ErrorCode(hr), FunctionName(functionName), FileName(fileName), LineNumber(lineNumber)
	{};
	std::wstring ToString() const;
	HRESULT ErrorCode;
	std::wstring FunctionName;
	std::wstring FileName;
	int LineNumber;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x) \
{ \
HRESULT hr__ = (x); \
std::wstring wfn = AnsiToWString(__FILE__); \
if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn,__LINE__); } \
}
#endif

template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isContantBuffer) : mIsConstantBuffer(isContantBuffer)
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
	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	inline ID3D12Resource* Resource() const
	{
		return mUploadBuffer.Get();
	}

private:
	ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData;
	UINT mElementByteSize;
	bool mIsConstantBuffer;
};

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

	UINT VertexByteStride= 0;
	UINT VertexBufferByteSize = 0;
	UINT IndexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
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


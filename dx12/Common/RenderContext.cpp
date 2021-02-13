#include "RenderContext.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include "Util.h"


RenderContext& RenderContext::Get()
{
    static RenderContext instance;
    return instance;
}

void RenderContext::Init()
{
    ComPtr<IDXGIFactory1> dxgiFactory;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
    ThrowIfFailed(dxgiFactory.As(&mDXGIFactory4));

    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;
    SIZE_T maxDedicatedVideoMemory = 0;
    for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
        dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

        // Check to see if the adapter can create a D3D12 device without actually 
        // creating it. The adapter with the largest dedicated video memory
        // is favored.
        if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
            SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
                                        D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
            dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
        {
            maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
            ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
        }
    }

    /*DXGI_QUERY_VIDEO_MEMORY_INFO local;
    DXGI_QUERY_VIDEO_MEMORY_INFO nonLocal;
    dxgiAdapter4->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local);
    dxgiAdapter4->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocal);*/

    //Create device
    //ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)));
    ThrowIfFailed(D3D12CreateDevice(dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)));

    //Check Tearing support
    ComPtr<IDXGIFactory5> factory5;
    ThrowIfFailed(dxgiFactory.As(&factory5));
    factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bAllowTearing, sizeof(bAllowTearing));

    //Create Command Queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));

    //Create Command Allocator
    mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));
    //Create Command List
    mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList));
    mCommandList->Close();
}

ID3D12Device2* RenderContext::Device()
{
    return mDevice.Get();
}

ID3D12CommandQueue* RenderContext::CommandQueue()
{
    return mCommandQueue.Get();
}

ID3D12GraphicsCommandList* RenderContext::CommandList()
{
    return mCommandList.Get();
}

ID3D12CommandAllocator* RenderContext::CommandAllocator()
{
    return mCommandAllocator.Get();
}

IDXGIFactory4* RenderContext::GetDXGIFactory()
{
    return mDXGIFactory4.Get();
}
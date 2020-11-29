#include "D3DApp.h"

#include<fstream>
#include <windowsx.h>
#include "../DX12HelloWorld/FrameResource.h"

#if defined(CreateWindow)
#undef CreateWindow
#endif

D3DApp::D3DApp(HINSTANCE hInstance) : 
    mhInstance(hInstance),
    mDsvDescriptorSize(0),
    mRtvDescriptorSize(0),
    mAllowTearing(false),
    mFence(nullptr),
    mDevice(nullptr),
    mCommandQueue(nullptr),
    mDXGIFactory4(nullptr),
    mClientWidth(0),
    mClientHeight(0),
    mCurrentBufferIndex(0),
    mFenceEvent(0),
    mFenceValue(0),
    mFenceValues{ 0 },
    mhMainWnd(0),
    mWindowClassName(L"Default"),
    mWindowTitleName(L"Default")
{}

D3DApp::~D3DApp()
{
    if (mDevice)
    {
        FlushCommandQueue();
    }
}

bool D3DApp::InitMainWindow(WNDPROC proc)
{

    WNDCLASSEXW windowClass = {};
    windowClass.hInstance = mhInstance;
    windowClass.lpszClassName = mWindowClassName;
    windowClass.lpfnWndProc = proc;
    windowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    windowClass.hIcon = ::LoadIcon(mhInstance, NULL);
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_VREDRAW | CS_HREDRAW;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpszMenuName = nullptr;
    windowClass.hIconSm = ::LoadIcon(mhInstance, nullptr);

    auto atom = ::RegisterClassExW(&windowClass);
    assert(atom > 0);

    mClientWidth = ::GetSystemMetrics(SM_CXSCREEN);
    mClientHeight = ::GetSystemMetrics(SM_CYSCREEN);

    UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

    mhMainWnd = ::CreateWindowExW(windowStyle,
                                    mWindowClassName,
                                    mWindowTitleName,
                                    WS_OVERLAPPEDWINDOW,
                                    0,
                                    0,
                                    mClientWidth,
                                    mClientHeight,
                                    nullptr,
                                    nullptr,
                                    mhInstance,
                                    nullptr);

    auto error = GetLastError();
    
    ::SetWindowLongW(mhMainWnd, GWL_STYLE, windowStyle);
    return mhMainWnd != 0;
}

bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG)
    //Enable Debug Layer
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif
    ComPtr<IDXGIFactory1> dxgiFactory;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
    ThrowIfFailed(dxgiFactory.As(&mDXGIFactory4));

    //Create device
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)));

    //Create Fence
    ThrowIfFailed(mDevice->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

    //Query heap size
    mDsvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    //Check Tearing support
    ComPtr<IDXGIFactory5> factory5;
    ThrowIfFailed(dxgiFactory.As(&factory5));
    factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &mAllowTearing, sizeof(mAllowTearing));

    CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();

    return true;
}

void D3DApp::CreateCommandObjects()
{
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

void D3DApp::CreateSwapChain()
{
    //Create swapchain
    mSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

    swapChainDesc.Width = mClientWidth;
    swapChainDesc.Height = mClientHeight;
    swapChainDesc.Format = mBackBufferFormat;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = mSwapChainBufferCount;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = mAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
;
    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(mDXGIFactory4->CreateSwapChainForHwnd(mCommandQueue.Get(), mhMainWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
    ThrowIfFailed(swapChain1.As(&mSwapChain));

    ThrowIfFailed(mDXGIFactory4->MakeWindowAssociation(mhMainWnd, DXGI_MWA_NO_ALT_ENTER));
}

int D3DApp::Run()
{
    ::ShowWindow(mhMainWnd, SW_MAXIMIZE);
    MSG msg = { 0 };

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Update();
        Render();
    }

    return 0;
}

bool D3DApp::Initialize(WNDPROC proc)
{
    if (InitMainWindow(proc) && InitDirect3D())
    {
        return true;
    }
    return false;
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg)
    {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_ESCAPE:
            ::PostQuitMessage(0);
            break;
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return  ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
    //Create Heaps
    /*D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.NumDescriptors = mSwapChainBufferCount * 2;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NodeMask = 0;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    mDevice->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&mRtvHeap));*/

    mRTBuffer = std::make_unique<ShaderResourceBuffer>(mDevice.Get(), mSwapChainBufferCount + 3, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
    dsvDesc.NumDescriptors = mSwapChainBufferCount;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.NodeMask = 0;
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    mDevice->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&mDsvHeap));

    //Create RTV
    //CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < mSwapChainBufferCount; ++i)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffers[i])));
        mRTHandles[i] = mRTBuffer->CpuHandle();
        mDevice->CreateRenderTargetView(mSwapChainBuffers[i].Get(), nullptr, mRTHandles[i]);
       // handle.Offset(mRtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc = { };

    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    depthStencilDesc.SampleDesc = { 1, 0 };

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1;
    optClear.DepthStencil.Stencil = 0;
    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(mDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(&mDepthStencilBuffer)));

    mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDsvHeap->GetCPUDescriptorHandleForHeapStart());
}

std::wstring DxException::ToString() const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + FileName + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

ComPtr<ID3D12Resource> D3DApp::CreateDefaultBuffer(const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())
        ));

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())
    ));

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(mCommandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    return defaultBuffer;

}

float D3DApp::AspectRatio() const
{
    return static_cast<float>(mClientWidth) / mClientHeight;
}

void D3DApp::FlushCommandQueue()
{
    // Advance the fence value to mark commands up to this fence point.
    mFenceValue++;

    // Add an instruction to the command queue to set a new fence point.  Because we 
    // are on the GPU timeline, the new fence point won't be set until the GPU finishes
    // processing all the commands prior to this Signal().
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));

    // Wait until the GPU has completed commands up to this fence point.
    if (mFence->GetCompletedValue() < mFenceValue)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

        // Fire event when GPU hits current fence.  
        ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValue, eventHandle));

        // Wait until the GPU hits current fence event is fired.
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

ComPtr<ID3DBlob> D3DApp::LoadBinary(const std::string& filename)
{
    std::ifstream fin(filename, std::ios::binary);
    ComPtr<ID3DBlob> blob;
    if (fin.is_open())
    {
        fin.seekg(0, std::ios_base::end);
        std::ifstream::pos_type size = (int)fin.tellg();
        fin.seekg(0, std::ios_base::beg);

        ThrowIfFailed(D3DCreateBlob(size, &blob));

        fin.read((char*)blob->GetBufferPointer(), size);
        fin.close();
    }
    
    return blob;
}
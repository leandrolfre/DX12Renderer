#include "D3DApp.h"

#include<fstream>
#include <windowsx.h>
#include "RenderContext.h"
#include "../DX12HelloWorld/ResourceManager.h"
//TODO:FIX ME
#include "../DX12HelloWorld/FrameResource.h"

#if defined(CreateWindow)
#undef CreateWindow
#endif

D3DApp::D3DApp(HINSTANCE hInstance) : 
    mhInstance(hInstance),
    mDsvDescriptorSize(0),
    mRtvDescriptorSize(0),
    mFence(nullptr),
    mCommandQueue(nullptr),
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
    FlushCommandQueue();
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

    RenderContext::Get().Init();

    auto Device = RenderContext::Get().Device();
    //Create Fence
    ThrowIfFailed(Device->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFence->SetName(L"Main Fence");

    //Query heap size
    mDsvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mRtvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();

    return true;
}

void D3DApp::CreateCommandObjects()
{
    auto& renderContext = RenderContext::Get();
    auto Device = renderContext.Device();
    mCommandQueue = renderContext.CommandQueue();
    mCommandAllocator = renderContext.CommandAllocator();
    mCommandList = renderContext.CommandList();
}

void D3DApp::CreateSwapChain()
{
    auto DXGIFactory = RenderContext::Get().GetDXGIFactory();
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
    swapChainDesc.Flags = RenderContext::Get().IsTearingAvailable() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
;
    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(DXGIFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), mhMainWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
    ThrowIfFailed(swapChain1.As(&mSwapChain));

    ThrowIfFailed(DXGIFactory->MakeWindowAssociation(mhMainWnd, DXGI_MWA_NO_ALT_ENTER));
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
    auto Device = RenderContext::Get().Device();
   
    for (int i = 0; i < mSwapChainBufferCount; ++i)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffers[i])));
        mBackbufferHandles[i] = ResourceManager::Get().AllocRenderTargetResource();
        Device->CreateRenderTargetView(mSwapChainBuffers[i].Get(), nullptr, mBackbufferHandles[i]);
        mSwapChainBuffers[i]->SetName(L"SwapChain " + i);
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
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(Device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &optClear, IID_PPV_ARGS(&mDepthStencilBuffer)));
    mDepthStencilBuffer->SetName(L"Depth Stencil Buffer");
    mDepthHandle = ResourceManager::Get().AllocDepthStencilResource();
    Device->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDepthHandle);
}

std::wstring DxException::ToString() const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + FileName + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
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
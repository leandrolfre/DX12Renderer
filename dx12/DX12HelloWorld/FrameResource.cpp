#include "FrameResource.h"
#include "..\Common\Util.h"
#include "..\Common\RenderContext.h"
#include "ResourceManager.h"

FrameResource::FrameResource(UINT passCount, UINT64 rtWidth, UINT64 rtHeight, DXGI_FORMAT rtFormat) : Fence(0)
{

    auto Device = RenderContext::Get().GetDevice();
    ThrowIfFailed(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdListAllocator.GetAddressOf())));
    UINT bufferSize = 100;
    PassCB = std::make_unique<UploadBuffer<PassConstants>>(Device, passCount, true);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(Device, bufferSize, true);
    MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(Device, bufferSize, false);

    RT[0] = std::make_unique<RenderTarget>(rtWidth, rtHeight, rtFormat);
    RT[1] = std::make_unique<RenderTarget>(rtWidth, rtHeight, rtFormat);

    RT[0]->Resource()->SetName(L"Frame Resource RT 0");
    RT[1]->Resource()->SetName(L"Frame Resource RT 1");
}

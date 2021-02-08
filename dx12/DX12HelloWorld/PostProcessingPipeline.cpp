#include "PostProcessingPipeline.h"

#include <d3d12.h>
#include "GeometryGenerator.h"
#include "Mesh.h"
#include "../Common/MathHelper.h"
#include "FrameResource.h"
#include "PostProcessing.h"

void PostProcessingPipeline::Init()
{
    GeometryGenerator GeoGen;
    //Generate mesh data (vertices and indexes)
    auto QuadData = GeoGen.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
    mScreenQuad = std::make_unique<Mesh>(QuadData,nullptr);
    
}

void PostProcessingPipeline::Run(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource)
{
    /*cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    cmdList->SetPipelineState(mPSOs["screen"].Get());
    cmdList->ClearRenderTargetView(renderTargetView, clearColor, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &renderTargetView, true, nullptr);
    cmdList->SetGraphicsRootSignature(mScreenRootSignature.Get());
    cmdList->SetGraphicsRootDescriptorTable(0, mCurrentFrameResource->RT[CurrentPingPingRT]->GpuSrv);*/
    cmdList->SetPipelineState(mScreenPso);
    for (int i = 0; i < mPostprocessingList.size(); ++i)
    {
        auto sourceRT = frameResource->RT[i % 2].get();
        auto targetRT = frameResource->RT[(i+1) % 2].get();
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sourceRT->Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(targetRT->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

        cmdList->OMSetRenderTargets(1, &targetRT->CpuRtv, true, nullptr);
        cmdList->SetGraphicsRootDescriptorTable(0, targetRT->GpuSrv);


        mPostprocessingList[i]->Run(sourceRT, cmdList);
    }
    auto objCB = frameResource->ObjectCB.get();
    mScreenQuad->Draw(cmdList, objCB);
}

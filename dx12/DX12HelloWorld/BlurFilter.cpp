#include "BlurFilter.h"
#include "FrameResource.h"
#include "../Common/Util.h"

BlurFilter::BlurFilter(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format) : mDevice{device}, mWidth{width}, mHeight{height}, mFormat{format}
{
    BuildResources();
}

void BlurFilter::BuildResources()
{
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));

    texDesc.Width = mWidth;
    texDesc.Height = mHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = mFormat;
    texDesc.Alignment = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&mBlurMap0)
    ));

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&mBlurMap1)
    ));
}

void BlurFilter::BuildDescriptors(ShaderResourceBuffer* srBuffer)
    //CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle, UINT descriptorSize
{
    mBlur0CpuSrv = srBuffer->CpuHandle();
    mBlur0CpuUav = srBuffer->CpuHandle();
    mBlur1CpuSrv = srBuffer->CpuHandle();
    mBlur1CpuUav = srBuffer->CpuHandle();

    mBlur0GpuSrv = srBuffer->GpuHandle();
    mBlur0GpuUav = srBuffer->GpuHandle();
    mBlur1GpuSrv = srBuffer->GpuHandle();
    mBlur1GpuUav = srBuffer->GpuHandle();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = mFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    mDevice->CreateShaderResourceView(mBlurMap0.Get(), &srvDesc, mBlur0CpuSrv);
    mDevice->CreateUnorderedAccessView(mBlurMap0.Get(), nullptr, &uavDesc, mBlur0CpuUav);

    mDevice->CreateShaderResourceView(mBlurMap1.Get(), &srvDesc, mBlur1CpuSrv);
    mDevice->CreateUnorderedAccessView(mBlurMap1.Get(), nullptr, &uavDesc, mBlur1CpuUav);
}

void BlurFilter::Execute(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig, ID3D12PipelineState* horzBlurPSO, ID3D12PipelineState* vertBlurPSO, ID3D12Resource* input, int blurCount)
{
    auto weights = CalcGaussWeights(2.5f);
    int blurRadius = (int)weights.size() / 2;

    cmdList->SetComputeRootSignature(rootSig);
    cmdList->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
    cmdList->SetComputeRoot32BitConstants(0, (UINT)weights.size(), weights.data(), 1);

    cmdList->ResourceBarrier(1,
                             &CD3DX12_RESOURCE_BARRIER::Transition(
                                 input,
                                 D3D12_RESOURCE_STATE_RENDER_TARGET,
                                 D3D12_RESOURCE_STATE_COPY_SOURCE));
    
    cmdList->ResourceBarrier(1,
                             &CD3DX12_RESOURCE_BARRIER::Transition(
                                 mBlurMap0.Get(),
                                 D3D12_RESOURCE_STATE_COMMON,
                                 D3D12_RESOURCE_STATE_COPY_DEST));

    cmdList->CopyResource(mBlurMap0.Get(), input);

    cmdList->ResourceBarrier(1,
                             &CD3DX12_RESOURCE_BARRIER::Transition(
                             mBlurMap0.Get(),
                                 D3D12_RESOURCE_STATE_COPY_DEST,
                                 D3D12_RESOURCE_STATE_GENERIC_READ));

    cmdList->ResourceBarrier(1,
                             &CD3DX12_RESOURCE_BARRIER::Transition(
                             mBlurMap1.Get(),
                                 D3D12_RESOURCE_STATE_COMMON,
                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    

    for(int i = 0; i < blurCount; ++i)
    {

        cmdList->SetPipelineState(horzBlurPSO);
        cmdList->SetComputeRootDescriptorTable(1, mBlur0GpuSrv);
        cmdList->SetComputeRootDescriptorTable(2, mBlur1GpuUav);

        UINT numGroupsX = (UINT)ceilf(mWidth / 256.0f);
        
        cmdList->Dispatch(numGroupsX, mHeight, 1);

        cmdList->ResourceBarrier(1,
                                 &CD3DX12_RESOURCE_BARRIER::Transition(
                                 mBlurMap0.Get(),
                                     D3D12_RESOURCE_STATE_GENERIC_READ,
                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

        cmdList->ResourceBarrier(1,
                                 &CD3DX12_RESOURCE_BARRIER::Transition(
                                     mBlurMap1.Get(),
                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                     D3D12_RESOURCE_STATE_GENERIC_READ));

        //Vert Blur
        cmdList->SetPipelineState(vertBlurPSO);
        cmdList->SetComputeRootDescriptorTable(1, mBlur1GpuSrv);
        cmdList->SetComputeRootDescriptorTable(2, mBlur0GpuUav);

        UINT numGroupsY = (UINT)ceilf(mHeight / 256.0f);
        cmdList->Dispatch(mWidth, numGroupsY, 1);

        cmdList->ResourceBarrier(1,
                                 &CD3DX12_RESOURCE_BARRIER::Transition(
                                     mBlurMap0.Get(),
                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                     D3D12_RESOURCE_STATE_GENERIC_READ));

        cmdList->ResourceBarrier(1,
                                 &CD3DX12_RESOURCE_BARRIER::Transition(
                                 mBlurMap1.Get(),
                                     D3D12_RESOURCE_STATE_GENERIC_READ,
                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    }

    // Prepare to copy blurred output to the back buffer.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
                                                                           D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

    cmdList->CopyResource(input, mBlurMap0.Get());
}

std::vector<float> BlurFilter::CalcGaussWeights(float sigma)
{
    float twoSigma2 = 2.0f * sigma * sigma;

    // Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
    // For example, for sigma = 3, the width of the bell curve is 
    int blurRadius = (int)ceil(2.0f * sigma);

    std::vector<float> weights;
    weights.resize(2 * blurRadius + 1);

    float weightSum = 0.0f;

    for (int i = -blurRadius; i <= blurRadius; ++i)
    {
        float x = (float)i;

        weights[i + blurRadius] = expf(-x * x / twoSigma2);

        weightSum += weights[i + blurRadius];
    }

    // Divide by the sum so all the weights add up to 1.0.
    for (int i = 0; i < weights.size(); ++i)
    {
        weights[i] /= weightSum;
    }

    return weights;
}

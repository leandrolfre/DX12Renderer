#pragma once

#include <d3d12.h>
#include <wrl.h>
#include "../Common/d3dx12.h"
#include <vector>

class ShaderResourceBuffer;

class BlurFilter
{
public:
    BlurFilter(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);
    void BuildDescriptors(ShaderResourceBuffer* srBuffer);
    void Execute(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig, ID3D12PipelineState* horzBlurPSO, ID3D12PipelineState* vertBlurPSO, ID3D12Resource* input, int blurCount);

private:
    void BuildResources();
    std::vector<float> CalcGaussWeights(float sigma);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap0;
    Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap1;
    ID3D12Device* mDevice;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuUav;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuUav;

    CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuUav;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuUav;

    UINT mWidth;
    UINT mHeight;
    DXGI_FORMAT mFormat;
};

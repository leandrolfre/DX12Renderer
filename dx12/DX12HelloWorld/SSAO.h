#pragma once
#include <wrl.h>
#include <memory>
#include <d3dcommon.h>
#include <DirectXPackedVector.h>
#include "../Common/d3dx12.h"
#include "FrameResource.h"


class RenderTarget;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
class Mesh;

using namespace Microsoft::WRL;

struct SSAOInputData
{
    RenderTarget* Position;
    RenderTarget* Normal;
    Mesh* Quad;
    UploadBuffer<ObjectConstants>* ObjectCB;
};

class SSAO
{
public:
    SSAO() = default;
    ~SSAO() = default;
    SSAO(const SSAO&) = delete;
    SSAO& operator=(const SSAO&) = delete;
    void init(UINT Width, UINT Height, const DirectX::XMFLOAT4X4& Projection);
    void run(const SSAOInputData& InputData);
    
public:
    CD3DX12_GPU_DESCRIPTOR_HANDLE SSAOMap;
private:
    DirectX::PackedVector::XMHALF4 mSSAONoise[16];
    UINT32 mWidth;
    UINT32 mHeight;
    std::unique_ptr<RenderTarget> mSSAOMap;
    ComPtr<ID3D12RootSignature> mRootSignature;
    ComPtr<ID3D12PipelineState> mPso;
    ComPtr<ID3D12Resource> mKernelUploader;
    ComPtr<ID3D12Resource> mKernelBufferGPU;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<ID3DBlob> mVShader;
    ComPtr<ID3DBlob> mPShader;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mNoiseHandle;
};


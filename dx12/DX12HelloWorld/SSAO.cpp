#include "SSAO.h"

#include "RenderTarget.h"
#include "../Common/RenderContext.h"
#include "ResourceManager.h"
#include "TextureManager.h"
#include "Mesh.h"
#include <DirectXMath.h>
#include <random>

void SSAO::init(UINT Width, UINT Height, const DirectX::XMFLOAT4X4& Projection)
{
    auto Device = RenderContext::Get().Device();
    mCommandList = RenderContext::Get().CommandList();

    mWidth = Width;
    mHeight = Height;
    
    //load shader
    mVShader = ResourceManager::Get().LoadBinary("Shaders/ssao_SD_vs.cso");
    mPShader = ResourceManager::Get().LoadBinary("Shaders/ssao_SD_ps.cso");

    //Create rootSignature
    CD3DX12_DESCRIPTOR_RANGE range[3];
    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //Position
    range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); //Normal
    range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); //Noise

    CD3DX12_ROOT_PARAMETER slotRootParameter[5];
    slotRootParameter[0].InitAsConstantBufferView(0); //quad Object
    slotRootParameter[1].InitAsConstantBufferView(1); //Sample offset
    slotRootParameter[2].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL); //SRVs
    slotRootParameter[3].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[4].InitAsDescriptorTable(1, &range[2], D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[2];
    StaticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    StaticSamplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    rootSigDesc.NumStaticSamplers = 2;
    rootSigDesc.pStaticSamplers = StaticSamplers;

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }

    ThrowIfFailed(hr);
    ThrowIfFailed(Device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));

    //create pso
    
    UINT byteOffset = 0;
    std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout =
    {
        {
            "POSITION", //Semantic Name 
            0, //Semantic Index  
            DXGI_FORMAT_R32G32B32_FLOAT, //Format
            0, //Input Slot
            byteOffset, //Aligned Byte Offset
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, //Input Slot Class
            0 // Instance DAta Step Rate
        },
        {
            "TANGENT",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            byteOffset += 12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "NORMAL",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            byteOffset += 12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "TEX",
            0,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            byteOffset += 12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "TEX",
            1,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            byteOffset += 8,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "COLOR",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            0,
            byteOffset += 8,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        }
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    psoDesc.InputLayout = { InputLayout.data(), static_cast<UINT>(InputLayout.size()) };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(mVShader->GetBufferPointer()), mVShader->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(mPShader->GetBufferPointer()), mPShader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = false;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ThrowIfFailed(Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPso)));

    //create CBV with random Samples over the hemisphere
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    auto lerp = [](float a, float b, float t) {
        return a + t * (b - a);
    };

    struct SSAOConstData
    {
        DirectX::XMFLOAT4X4 Projection;
        DirectX::XMFLOAT4 kernel[64];
    };

    SSAOConstData constData;
    constData.Projection = Projection;
    for (int i = 0; i < 64; ++i)
    {
        DirectX::XMVECTOR Sample = DirectX::XMVectorSet
        (
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator),
            0.0f
        );

        Sample = DirectX::XMVector4Normalize(Sample);
        Sample = DirectX::XMVectorScale(Sample, randomFloats(generator));
        float scale = (float)i / 64.0f;
        scale = lerp(0.1f, 1.0f, scale * scale);
        Sample = DirectX::XMVectorScale(Sample, scale);
        DirectX::XMFLOAT4 KernelSample;
        DirectX::XMStoreFloat4(&KernelSample, Sample);
        constData.kernel[i] = KernelSample;
    }
    
    auto byteSize = ((sizeof(SSAOConstData) + 255) & ~255);
    mKernelBufferGPU = ResourceManager::Get().CreateDefaultBuffer(&constData, byteSize, mKernelUploader, RenderContext::Get().CommandList());

    //create SRV noise texture
    for (int i = 0; i < 16; ++i)
    {
        mSSAONoise[i] = DirectX::PackedVector::XMHALF4(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f,
            0.0f);
    }

    Texture* NoiseTexture = TextureManager::Get().CreateTexture(L"NoiseTexture", 4, 4, DXGI_FORMAT_R16G16B16A16_FLOAT, &mSSAONoise, 64);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    ZeroMemory(&srvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.Texture2D.MipLevels = 0;
    srvDesc.Format = NoiseTexture->Resource->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = NoiseTexture->Resource->GetDesc().MipLevels;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    Device->CreateShaderResourceView(NoiseTexture->Resource.Get(), &srvDesc, ResourceManager::Get().AllocShaderResource());
    mNoiseHandle = ResourceManager::Get().AllocGPUShaderResource();

    mSSAOMap = std::make_unique<RenderTarget>(mWidth, mHeight, DXGI_FORMAT_R16_FLOAT);
    SSAOMap = mSSAOMap->GpuSrv;
}

void SSAO::run(const SSAOInputData& InputData)
{
    D3D12_VIEWPORT screenViewport = { 0.0f, 0.0f, static_cast<FLOAT>(mWidth), static_cast<FLOAT>(mHeight), 0.0f, 1.0f };
    mCommandList->RSSetViewports(1, &screenViewport);
    RECT scissorRect = { 0L, 0L, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };
    mCommandList->RSSetScissorRects(1, &scissorRect);
    
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSSAOMap->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    mCommandList->ClearRenderTargetView(mSSAOMap->CpuRtv, clearColor, 0, nullptr);
    mCommandList->OMSetRenderTargets(1, &mSSAOMap->CpuRtv, false, nullptr);

    
    
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->SetPipelineState(mPso.Get());
    
    //Bind resources
    mCommandList->SetGraphicsRootConstantBufferView(1, mKernelBufferGPU->GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootDescriptorTable(2, InputData.Position->GpuSrv);
    mCommandList->SetGraphicsRootDescriptorTable(3, InputData.Normal->GpuSrv);
    mCommandList->SetGraphicsRootDescriptorTable(4, mNoiseHandle);

    InputData.Quad->Draw(mCommandList.Get(), InputData.ObjectCB);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSSAOMap->Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

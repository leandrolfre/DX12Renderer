#include "HelloWorldApp.h"
#include "GeometryGenerator.h"
#include "TextureManager.h"

#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx12.h"
#include "../Common/RenderContext.h"
#include "ResourceManager.h"
#include "pix3.h"
#include <algorithm>

static const int gNumFrameResources = 3;

bool D3DAppHelloWorld::Initialize(WNDPROC proc)
{
    if (!D3DApp::Initialize(proc))
    {
        return false;
    }

    auto Device = RenderContext::Get().Device();
    mCam.SetPosition(4.0f, 10.0f, -15.0f);
    mCam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

    mShadowMap = std::make_unique<ShadowMap>(4096, 4096);

    mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
    mSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);

    //mBlurFilter = std::make_unique<BlurFilter>(Device, mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

    BuildRootSignature();
    BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    BuildMaterials();
    BuildScene();
    BuildFrameResources();
    BuildPSO();

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
  
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(mhMainWnd);
    ImGui_ImplDX12_Init(Device, 
                        gNumFrameResources,
                        DXGI_FORMAT_R8G8B8A8_UNORM,
                        mUISrvDescriptorHeap.Get(),
                        mUISrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                        mUISrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}

void D3DAppHelloWorld::BuildDescriptorHeaps()
{
    auto Device = RenderContext::Get().Device();
   // mBlurFilter->BuildDescriptors();

    D3D12_DESCRIPTOR_HEAP_DESC descUI = {};
    descUI.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descUI.NumDescriptors = 1;
    descUI.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(Device->CreateDescriptorHeap(&descUI, IID_PPV_ARGS(&mUISrvDescriptorHeap)));
    mUISrvDescriptorHeap->SetName(L"UI Descriptor Heap");

}

void CreateRootSignature(ComPtr<ID3D12Device2> Device, const CD3DX12_ROOT_SIGNATURE_DESC& RootSignatureDesc, ID3D12RootSignature** RootSignature)
{
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }

    ThrowIfFailed(hr);
    ThrowIfFailed(Device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(RootSignature)));
}

void D3DAppHelloWorld::BuildDefaultRootSignature()
{
    auto Device = RenderContext::Get().Device();
    CD3DX12_DESCRIPTOR_RANGE range[3];
    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); //cube
    range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0); //shadow
    range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 2, 0); //Object textures

    CD3DX12_ROOT_PARAMETER slotRootParameter[6];
    slotRootParameter[0].InitAsConstantBufferView(0); //Object
    slotRootParameter[1].InitAsDescriptorTable(1, &range[2], D3D12_SHADER_VISIBILITY_PIXEL); //Object Textures
    slotRootParameter[2].InitAsShaderResourceView(0, 1); //Material
    slotRootParameter[3].InitAsConstantBufferView(1); //Pass
    slotRootParameter[4].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL); //CubeMap
    slotRootParameter[5].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL); //ShadowMap
    


    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(6, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[2];
    StaticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    StaticSamplers[1].Init(1,
                           D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
                           D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                           D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                           D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                           0.0f, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL,
                           D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);
    rootSigDesc.NumStaticSamplers = 2;
    rootSigDesc.pStaticSamplers = StaticSamplers;

    CreateRootSignature(Device, rootSigDesc, mRootSignature.GetAddressOf());
}

void D3DAppHelloWorld::BuildBlurRootSignature()
{
    auto Device = RenderContext::Get().Device();
    CD3DX12_DESCRIPTOR_RANGE blurRange[2];
    blurRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    blurRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    CD3DX12_ROOT_PARAMETER slotBlurRootParameter[3];
    slotBlurRootParameter[0].InitAsConstants(12, 0);
    slotBlurRootParameter[1].InitAsDescriptorTable(1, &blurRange[0]);
    slotBlurRootParameter[2].InitAsDescriptorTable(1, &blurRange[1]);

    CD3DX12_ROOT_SIGNATURE_DESC rootBlurSigDesc(3, slotBlurRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    CreateRootSignature(Device, rootBlurSigDesc, mPostProcessRootSignature.GetAddressOf());
}

void D3DAppHelloWorld::BuildScreenRootSignature()
{
    auto Device = RenderContext::Get().Device();
    CD3DX12_DESCRIPTOR_RANGE table;
    table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    
    CD3DX12_ROOT_PARAMETER rootParameter[2];
    rootParameter[0].InitAsConstantBufferView(0); //Object
    rootParameter[1].InitAsDescriptorTable(1, &table, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rootScreenSigDesc(2, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[1];
    StaticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    rootScreenSigDesc.NumStaticSamplers = 1;
    rootScreenSigDesc.pStaticSamplers = StaticSamplers;

    CreateRootSignature(Device, rootScreenSigDesc, mScreenRootSignature.GetAddressOf());
}

void D3DAppHelloWorld::BuildRootSignature()
{
    BuildDefaultRootSignature();
    BuildBlurRootSignature();
    BuildScreenRootSignature();
}

void D3DAppHelloWorld::BuildShadersAndInputLayout()
{
    mvsByteCode = LoadBinary("Shaders/color_SD_vs.cso");
    mpsByteCode = LoadBinary("Shaders/color_SD_ps.cso");
    mcsHorizontalByteCode = LoadBinary("Shaders/horzBlur_CS_cs.cso");
    mcsVerticalByteCode = LoadBinary("Shaders/vertBlur_CS_cs.cso");
    mSkyVsByteCode = LoadBinary("Shaders/sky_SD_vs.cso");
    mSkyPsByteCode = LoadBinary("Shaders/sky_SD_ps.cso");

    mQuadVsByteCode = LoadBinary("Shaders/quad_SD_vs.cso");
    mQuadPsByteCode = LoadBinary("Shaders/quad_SD_ps.cso");

    mShadowVSByteCode = LoadBinary("Shaders/shadow_SD_vs.cso");
    UINT byteOffset = 0;
    mInputLayout =
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
            byteOffset+=12,
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
}

void D3DAppHelloWorld::BuildPSO()
{
    auto Device = RenderContext::Get().Device();
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    psoDesc.InputLayout = { mInputLayout.data(), static_cast<UINT>(mInputLayout.size()) };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), mvsByteCode->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()), mpsByteCode->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ThrowIfFailed(Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowDesc = psoDesc;
    shadowDesc.RasterizerState.DepthBias = 100000;
    shadowDesc.RasterizerState.DepthBiasClamp = 0.0f;
    shadowDesc.RasterizerState.SlopeScaledDepthBias = 5.0f;
    shadowDesc.pRootSignature = mRootSignature.Get();
    shadowDesc.VS = { reinterpret_cast<BYTE*>(mShadowVSByteCode->GetBufferPointer()), mShadowVSByteCode->GetBufferSize() };
    shadowDesc.PS = {};
    shadowDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    shadowDesc.NumRenderTargets = 0;

    ThrowIfFailed(Device->CreateGraphicsPipelineState(&shadowDesc, IID_PPV_ARGS(&mPSOs["shadow_opaque"])));
    

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = psoDesc;
    opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(Device->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = psoDesc;
    skyPsoDesc.pRootSignature = mRootSignature.Get();
    skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    skyPsoDesc.VS = { reinterpret_cast<BYTE*>(mSkyVsByteCode->GetBufferPointer()), mSkyVsByteCode->GetBufferSize() };
    skyPsoDesc.PS = { reinterpret_cast<BYTE*>(mSkyPsByteCode->GetBufferPointer()), mSkyPsByteCode->GetBufferSize() };
    ThrowIfFailed(Device->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC screenPsoDesc = psoDesc;
    screenPsoDesc.pRootSignature = mScreenRootSignature.Get();
    screenPsoDesc.RasterizerState.DepthClipEnable = false;
    screenPsoDesc.RasterizerState.DepthClipEnable = false;
    screenPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    screenPsoDesc.VS = { reinterpret_cast<BYTE*>(mQuadVsByteCode->GetBufferPointer()), mQuadVsByteCode->GetBufferSize() };
    screenPsoDesc.PS = { reinterpret_cast<BYTE*>(mQuadPsByteCode->GetBufferPointer()), mQuadPsByteCode->GetBufferSize() };

    ThrowIfFailed(Device->CreateGraphicsPipelineState(&screenPsoDesc, IID_PPV_ARGS(&mPSOs["screen"])));

    D3D12_COMPUTE_PIPELINE_STATE_DESC horizontalBlurDesc;
    ZeroMemory(&horizontalBlurDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
    horizontalBlurDesc.pRootSignature = mPostProcessRootSignature.Get();
    horizontalBlurDesc.CS = {reinterpret_cast<BYTE*>(mcsHorizontalByteCode->GetBufferPointer()), mcsHorizontalByteCode->GetBufferSize()};
    horizontalBlurDesc.NodeMask = 0;
    horizontalBlurDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(Device->CreateComputePipelineState(&horizontalBlurDesc, IID_PPV_ARGS(&mPSOs["horzBlur"])));
    
    D3D12_COMPUTE_PIPELINE_STATE_DESC verticalBlurDesc = {};
    ZeroMemory(&verticalBlurDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
    verticalBlurDesc.pRootSignature = mPostProcessRootSignature.Get();
    verticalBlurDesc.CS = { reinterpret_cast<BYTE*>(mcsVerticalByteCode->GetBufferPointer()), mcsVerticalByteCode->GetBufferSize() };
    verticalBlurDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(Device->CreateComputePipelineState(&verticalBlurDesc, IID_PPV_ARGS(&mPSOs["vertBlur"])));
}

void D3DAppHelloWorld::BuildFrameResources()
{
    auto Device = RenderContext::Get().Device();

    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(1, mClientWidth, mClientHeight, mBackBufferFormat));
    }
}

void D3DAppHelloWorld::BuildScene()
{
    GeometryGenerator geoGen;
    //auto totalVertexCount = circle.Vertices.size() + grid.Vertices.size() + lightBox.Vertices.size();

    std::unordered_map<std::string, MeshData> meshes = {
        {"sphere", geoGen.CreateSphere(1, 500, 100) }
       ,{"floor", geoGen.CreateGrid(20.0f, 30.0f, 2, 2)}
       ,{"lightBox", geoGen.CreateBox(5.0f, 5.0f, 5.0f, 1)}
    };

    for (auto& meshData : meshes)
    {
        auto mesh = std::make_unique<Mesh>(meshData.second, mMaterials[meshData.first].get());
        
        if (meshData.first == "sphere")
        {
            XMStoreFloat4x4(&mesh->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 3.0f));
        }
        else if (meshData.first == "lightBox")
        {
            XMFLOAT3 lightPos;
            XMStoreFloat3(&lightPos, MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi));
            XMStoreFloat4x4(&mesh->World, XMMatrixScaling(0.04f, 0.04f, 0.04f) * XMMatrixTranslation(lightPos.x, lightPos.y * 7.0f, lightPos.z));
            mLightBox = mesh.get();
        }
        mesh->UploadData(mCommandList.Get());
        mLayerRItems[(int)RenderLayer::Opaque].push_back(std::move(mesh));
    }

    auto skyRItem = std::make_unique<Mesh>(meshes["sphere"], mMaterials["sky"].get());
    XMStoreFloat4x4(&skyRItem->World, XMMatrixScaling(50.0f, 50.0f, 50.0f));
    skyRItem->UploadData(mCommandList.Get());
    mLayerRItems[(int)RenderLayer::Sky].push_back(std::move(skyRItem));

    auto quadData = geoGen.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
    auto fullscreenQuad = std::make_unique<Mesh>(quadData, nullptr);
    fullscreenQuad->UploadData(mCommandList.Get());

    mLayerRItems[int(RenderLayer::Fullscreen)].push_back(std::move(fullscreenQuad));
}


CD3DX12_GPU_DESCRIPTOR_HANDLE CreateTextureSRV(const std::wstring& path, bool isCubeMap)
{
    auto Device = RenderContext::Get().Device();
    auto& ResourceManager = ResourceManager::Get();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    auto Texture = !path.empty() ? TextureManager::Get().getTexture(path) : nullptr;
    ID3D12Resource* Resource = nullptr;

    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.Texture2D.MipLevels = 0;
    if (Texture)
    {
        srvDesc.Format = Texture->Resource->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = Texture->Resource->GetDesc().MipLevels;
        Resource = Texture->Resource.Get();
    }
    
    srvDesc.ViewDimension = isCubeMap ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    auto srvHandle = ResourceManager.AllocShaderResource();
    Device->CreateShaderResourceView(Resource, &srvDesc, srvHandle);
    return ResourceManager.AllocGPUShaderResource();
}

void D3DAppHelloWorld::BuildMaterials()
{

    std::vector<std::wstring> texPaths = {
        L"Texture/pbr/beaten-up-metal1-albedo.dds",
        L"Texture/pbr/beaten-up-metal1-Normal-dx.dds",
        L"Texture/bricks2.dds",
        L"Texture/bricks2_nmap.dds",
        L"Texture/hsquareCM.dds"
    };

    auto floor = std::make_unique<Material>();
    floor->Name = "floor";
    floor->NumFramesDirty = gNumFrameResources;
    floor->MatCBIndex = 1;
    floor->DiffuseAlbedo = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    floor->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    floor->Roughness = 0.05f;
    floor->TextureHandle = CreateTextureSRV(texPaths[2], false);
    CreateTextureSRV(texPaths[3], false);
    floor->hasNormalMap = 0;
    

    auto lightbox = std::make_unique<Material>();
    lightbox->Name = "lightBox";
    lightbox->NumFramesDirty = gNumFrameResources;
    lightbox->MatCBIndex = 3;
    lightbox->DiffuseAlbedo = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
    lightbox->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    lightbox->Roughness = 0.05f;
    lightbox->TextureHandle = CreateTextureSRV(texPaths[2], false);
    CreateTextureSRV(L"", false);
    lightbox->hasNormalMap = 0;

    auto sphere = std::make_unique<Material>();
    sphere->Name = "spheres";
    sphere->NumFramesDirty = gNumFrameResources;
    sphere->MatCBIndex = 0;
    sphere->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
    sphere->FresnelR0 = XMFLOAT3(1.0f, 1.0f, 1.0f);
    sphere->Roughness = 0.05f;
    sphere->TextureHandle = CreateTextureSRV(texPaths[0], false);
    CreateTextureSRV(texPaths[1], false);
    sphere->hasNormalMap = 1;

    auto sky = std::make_unique<Material>();
    sky->Name = "sky";
    sky->NumFramesDirty = gNumFrameResources;
    sky->MatCBIndex = 2;
    sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
    sky->FresnelR0 = XMFLOAT3(0.95f, 0.93f, 0.88f);
    sky->Roughness = 0.05f;
    sky->TextureHandle = CreateTextureSRV(texPaths[4], true);
    CreateTextureSRV(L"", false);
    sky->hasNormalMap = 0;
    mCubeMapHandle = sky->TextureHandle;
    
    mMaterials["sphere"] = std::move(sphere);
    mMaterials["floor"] = std::move(floor);
    mMaterials["lightBox"] = std::move(lightbox);
    mMaterials["sky"] = std::move(sky);
}

void D3DAppHelloWorld::UpdateMaterialCB()
{
    auto currMaterialCB = mCurrentFrameResource->MaterialCB.get();
    for (auto& e : mMaterials)
    {
        Material* mat = e.second.get();
        if (mat && mat->NumFramesDirty > 0)
        {
            /*XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);*/
            MaterialConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;
            matConstants.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
            matConstants.hasNormalMap = mat->hasNormalMap;
            currMaterialCB->CopyData(mat->MatCBIndex,
                                     matConstants);
            // Next FrameResource need to be updated too.
            mat->NumFramesDirty--;
        }
    }
}

void D3DAppHelloWorld::UpdateMainPassCB()
{

    XMMATRIX view = mCam.GetView();
    XMMATRIX proj = mCam.GetProj();
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
    
    XMVECTOR lightDir = MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
    XMVECTOR lightPos = 2.0f*mSceneBounds.Radius*lightDir;
    XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

    XMFLOAT3 sphereCenterLightSpace;
    XMStoreFloat3(&sphereCenterLightSpace, XMVector3Transform(targetPos, lightView));

    float l = sphereCenterLightSpace.x - mSceneBounds.Radius;
    float b = sphereCenterLightSpace.y - mSceneBounds.Radius;
    float n = sphereCenterLightSpace.z - mSceneBounds.Radius;
    float r = sphereCenterLightSpace.x + mSceneBounds.Radius;
    float t = sphereCenterLightSpace.y + mSceneBounds.Radius;
    float f = sphereCenterLightSpace.z + mSceneBounds.Radius;

    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    );

    XMMATRIX S = lightView * lightProj * T;
    //XMStoreFloat4x4(&mLightBox->World, XMMatrixScaling(0.04f, 0.04f, 0.04f) * XMMatrixTranslation(lightPos, lightPos.y * 7.0f, lightPos.z));
    
    XMStoreFloat4x4(&mMainPassCB.View, view);
    XMStoreFloat4x4(&mMainPassCB.InvView, invView);
    XMStoreFloat4x4(&mMainPassCB.Proj, proj);
    XMStoreFloat4x4(&mMainPassCB.InvProj, invProj);
    XMStoreFloat4x4(&mMainPassCB.ViewProj, viewProj);
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, invViewProj);
    XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
    XMStoreFloat4x4(&mMainPassCB.Lights[0].ShadowViewProj, XMMatrixMultiply(lightView, lightProj));
    XMStoreFloat4x4(&mMainPassCB.Lights[0].ShadowTransform, S);

    mMainPassCB.EyePosW = mCam.GetPosition3f();
    mMainPassCB.RenderTargetSize = XMFLOAT2(static_cast<float>(mClientWidth), static_cast<float>(mClientHeight));
    mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = 0.0f;
    mMainPassCB.DeltaTime = 0.0f;
    mMainPassCB.AmbientLight = { 0.05f, 0.05f, 0.05f, 1.0f };
    mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 1.0f };

    auto currPassCB = mCurrentFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void D3DAppHelloWorld::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<std::unique_ptr<Mesh>>& rItems)
{   
    auto objCB = mCurrentFrameResource->ObjectCB.get();
    for (int i = 0; i < rItems.size(); ++i)
    {
        rItems[i]->Draw(cmdList, objCB);
    }
}

void D3DAppHelloWorld::Update()
{
    OnKeyboardInput();
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % gNumFrameResources;
    mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

    if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence)
    {
        auto fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
        mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, fenceEvent);
        if (fenceEvent)
        {
            WaitForSingleObject(fenceEvent, INFINITE);
            CloseHandle(fenceEvent);
        }
    }

    UpdateMaterialCB();
    UpdateMainPassCB();
}

void D3DAppHelloWorld::Render()
{
    auto cmdListAlloc = mCurrentFrameResource->CmdListAllocator;
    ThrowIfFailed(cmdListAlloc->Reset());
    if (mIsWireframe)
    {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
    }
    else
    {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
    }

    mCurrentFrameResource->ObjectCB->Reset();

    ////////// UI TEST /////////
     // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static int blurPasses = 1;
    static float reflective = mMaterials["sphere"]->FresnelR0.x;
    static float roughness = mMaterials["sphere"]->Roughness;
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    {
        ImGui::Begin("Test");                          // Create a window called "Hello, world!" and append into it.
        //ImGui::Checkbox("Blur", &mIsBlurEnabled);      // Edit bools storing our window open/close state
        ImGui::SliderInt("Blur Passes", &blurPasses, 1, 20);
        if (ImGui::SliderFloat("Reflection", &reflective, 0.0f, 1.0f))
        {
            mMaterials["sphere"]->FresnelR0.x = reflective;
            mMaterials["sphere"]->FresnelR0.y = reflective;
            mMaterials["sphere"]->FresnelR0.z = reflective;
            mMaterials["sphere"]->NumFramesDirty = 3;

            mMaterials["floor"]->FresnelR0.x = reflective;
            mMaterials["floor"]->FresnelR0.y = reflective;
            mMaterials["floor"]->FresnelR0.z = reflective;
            mMaterials["floor"]->NumFramesDirty = 3;
        }

        if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
        {
            mMaterials["sphere"]->Roughness = roughness;
            mMaterials["sphere"]->NumFramesDirty = 3;
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }
    ///////////////////////////

    ShadowPass();

    BeginRender();
    
    BasePass();
    
    //postprocessingPipeline.run();
    
    CompositePass();
    
    ID3D12DescriptorHeap* UIDescriptorHeaps[] = { mUISrvDescriptorHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(UIDescriptorHeaps), UIDescriptorHeaps);
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

    EndRender();
}

void D3DAppHelloWorld::OnKeyboardInput()
{
    if (GetAsyncKeyState('W') & 0x8000)
    {
        mCam.Walk(0.2f);
    }

    if (GetAsyncKeyState('S') & 0x8000)
    {
        mCam.Walk(-0.2f);
    }

    if (GetAsyncKeyState('A') & 0x8000)
    {
        mCam.Strafe(-0.2f);
    }

    if (GetAsyncKeyState('D') & 0x8000)
    {
        mCam.Strafe(0.2f);
    }

    if (GetAsyncKeyState(VK_LEFT) & 0x8000)
        mSunTheta -= 0.01f;

    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
        mSunTheta += 0.01f;

    if (GetAsyncKeyState(VK_UP) & 0x8000)
        mSunPhi -= 0.01f;

    if (GetAsyncKeyState(VK_DOWN) & 0x8000)
        mSunPhi += 0.01f;

        
    mSunPhi = MathHelper::Clamp(mSunPhi, 0.1f, XM_PIDIV2);
    mCam.UpdateViewMatrix();
}

void D3DAppHelloWorld::BasePass()
{
    PIXBeginEvent(mCommandList.Get(), PIX_COLOR(255, 0, 0), "Base Pass");
    mCommandList->OMSetRenderTargets(1, &mCurrentFrameResource->RT[0]->CpuRtv, true, &mDepthHandle);

    ID3D12DescriptorHeap* descriptorHeaps[] = { ResourceManager::Get().DescriptorHeaps()/*, mUISrvDescriptorHeap.Get()*/ };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    //bind all materials
    auto matCB = mCurrentFrameResource->MaterialCB->Resource();
    mCommandList->SetGraphicsRootShaderResourceView(2, matCB->GetGPUVirtualAddress());

    auto passCB = mCurrentFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());

    //Save cubemap handle info elsewhere
    mCommandList->SetGraphicsRootDescriptorTable(4, mCubeMapHandle);

    mCommandList->SetGraphicsRootDescriptorTable(5, mShadowMap->Srv());

    mCommandList->SetPipelineState(mPSOs["opaque"].Get());
    DrawRenderItems(mCommandList.Get(), mLayerRItems[(int)RenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs["sky"].Get());
    DrawRenderItems(mCommandList.Get(), mLayerRItems[(int)RenderLayer::Sky]);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
                                                                           D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON));
    PIXEndEvent();
}

void D3DAppHelloWorld::CompositePass()
{
    PIXBeginEvent(mCommandList.Get(), PIX_COLOR(255, 0, 0), "Composite Pass");
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCurrentFrameResource->RT[0]->Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
    mCommandList->SetGraphicsRootSignature(mScreenRootSignature.Get());
    mCommandList->SetPipelineState(mPSOs["screen"].Get());
    mCommandList->SetGraphicsRootDescriptorTable(1, mCurrentFrameResource->RT[0]->GpuSrv);
    mCommandList->OMSetRenderTargets(1, &mBackbufferHandles[mCurrentBufferIndex], true, nullptr);

    DrawRenderItems(mCommandList.Get(), mLayerRItems[(int)RenderLayer::Fullscreen]);
    PIXEndEvent();
}

void D3DAppHelloWorld::ShadowPass()
{
    PIXBeginEvent(mCommandList.Get(), PIX_COLOR(255, 0, 0), "Shadow Pass");
    mCommandList->RSSetViewports(1, &mShadowMap->Viewport());
    mCommandList->RSSetScissorRects(1, &mShadowMap->ScissorRect());

    ID3D12DescriptorHeap* descriptorHeaps[] = { ResourceManager::Get().DescriptorHeaps()/*, mUISrvDescriptorHeap.Get()*/ };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
    mCommandList->ClearDepthStencilView(mShadowMap->Dsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->Dsv());
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    auto passCB = mCurrentFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());
    mCommandList->SetPipelineState(mPSOs["shadow_opaque"].Get());
    DrawRenderItems(mCommandList.Get(), mLayerRItems[(int)RenderLayer::Opaque]);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
    PIXEndEvent();
}

void D3DAppHelloWorld::BeginRender()
{
    auto currentBackBuffer = mSwapChainBuffers[mCurrentBufferIndex].Get();
    auto renderTargetView = mBackbufferHandles[mCurrentBufferIndex];
    auto depthStencilView = mDepthHandle;
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    D3D12_VIEWPORT screenViewport = { 0.0f, 0.0f, static_cast<FLOAT>(mClientWidth), static_cast<FLOAT>(mClientHeight), 0.0f, 1.0f };
    mCommandList->RSSetViewports(1, &screenViewport);

    RECT scissorRect = { 0L, 0L, static_cast<LONG>(mClientWidth), static_cast<LONG>(mClientHeight) };
    mCommandList->RSSetScissorRects(1, &scissorRect);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCurrentFrameResource->RT[0]->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    // Transition the resource from its initial state to be used as a depth buffer.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
                                                                           D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    mCommandList->ClearRenderTargetView(mCurrentFrameResource->RT[0]->CpuRtv, clearColor, 0, nullptr);
    mCommandList->ClearRenderTargetView(renderTargetView, clearColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void D3DAppHelloWorld::EndRender()
{
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffers[mCurrentBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* commandLists[] = { mCommandList.Get() };

    mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    mSwapChain->Present(0, RenderContext::Get().IsTearingAvailable() ? DXGI_PRESENT_ALLOW_TEARING : 0);

    mCurrentFrameResource->Fence = ++mFenceValue;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));
    mCurrentBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void D3DAppHelloWorld::OnMouseMove(WPARAM	btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        mCam.Pitch(dy);
        mCam.RotateY(dx);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT	D3DAppHelloWorld::MsgProc(HWND hwnd, UINT	msg, WPARAM wParam, LPARAM	lParam)
{
    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGuiIO& io = ImGui::GetIO();
        if ((io.WantCaptureMouse || io.WantCaptureKeyboard) && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam) == 0)
            return true;
    }

    D3DApp::MsgProc(hwnd, msg, wParam, lParam);

}
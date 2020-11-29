#include "HelloWorldApp.h"
#include "GeometryGenerator.h"
#include "../Common/DDSTextureLoader.h"

#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx12.h"

static const int gNumFrameResources = 3;

bool D3DAppHelloWorld::Initialize(WNDPROC proc)
{
    if (!D3DApp::Initialize(proc))
    {
        return false;
    }
    mCam.SetPosition(4.0f, 10.0f, -15.0f);
    mCam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    mBlurFilter = std::make_unique<BlurFilter>(mDevice.Get(), mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

    BuildRootSignature();
    BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
    BuildTextures();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSO();

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
  
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(mhMainWnd);
    ImGui_ImplDX12_Init(mDevice.Get(), 
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
    mSRBuffer = std::make_unique<ShaderResourceBuffer>(mDevice.Get(), 100, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    mBlurFilter->BuildDescriptors(mSRBuffer.get());

    D3D12_DESCRIPTOR_HEAP_DESC descUI = {};
    descUI.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descUI.NumDescriptors = 1;
    descUI.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&descUI, IID_PPV_ARGS(&mUISrvDescriptorHeap)));

}

void D3DAppHelloWorld::BuildConstantBuffers()
{
    UINT objBufSize = ((sizeof(ObjectConstants) + 255) & ~255);
    UINT passCBByteSize = ((sizeof(PassConstants) + 255) & ~255);
    UINT objCount = mAllItems.size();
    auto handleHeapStart = mCbvHeap->GetCPUDescriptorHandleForHeapStart();
    for(int frameIdx = 0; frameIdx < gNumFrameResources; ++frameIdx)
    {
        auto passCB = mFrameResources[frameIdx]->PassCB->Resource();
        auto passCBAddress = passCB->GetGPUVirtualAddress();
        auto passHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(handleHeapStart, mPassCbvOffset + frameIdx, mSRBuffer->DescriptorHeapSize());

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc;
        cbViewDesc.BufferLocation = passCBAddress;
        cbViewDesc.SizeInBytes = passCBByteSize;
        mDevice->CreateConstantBufferView(&cbViewDesc, passHandle);

        auto objCB = mFrameResources[frameIdx]->ObjectCB->Resource();

        for(int i = 0; i < objCount; ++i)
        {
            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objCB->GetGPUVirtualAddress();
            cbAddress += i * objBufSize;
            int heapIndex = frameIdx * objCount + i;

            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(handleHeapStart, heapIndex, mSRBuffer->DescriptorHeapSize());

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc;
            cbViewDesc.BufferLocation = cbAddress;
            cbViewDesc.SizeInBytes = objBufSize;
            mDevice->CreateConstantBufferView(&cbViewDesc, handle);
        }
    }
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
    CD3DX12_DESCRIPTOR_RANGE range[2];
    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[5];
    slotRootParameter[0].InitAsConstantBufferView(0); //Object
    slotRootParameter[1].InitAsConstantBufferView(1); //Pass
    slotRootParameter[2].InitAsShaderResourceView(0, 1); //Material
    slotRootParameter[3].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL); //CubeMap
    slotRootParameter[4].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL); //Textures


    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[1];
    StaticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = StaticSamplers;

    CreateRootSignature(mDevice, rootSigDesc, mRootSignature.GetAddressOf());
}

void D3DAppHelloWorld::BuildBlurRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE blurRange[2];
    blurRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    blurRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    CD3DX12_ROOT_PARAMETER slotBlurRootParameter[3];
    slotBlurRootParameter[0].InitAsConstants(12, 0);
    slotBlurRootParameter[1].InitAsDescriptorTable(1, &blurRange[0]);
    slotBlurRootParameter[2].InitAsDescriptorTable(1, &blurRange[1]);

    CD3DX12_ROOT_SIGNATURE_DESC rootBlurSigDesc(3, slotBlurRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    CreateRootSignature(mDevice, rootBlurSigDesc, mPostProcessRootSignature.GetAddressOf());
}

void D3DAppHelloWorld::BuildScreenRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE table;
    table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    
    CD3DX12_ROOT_PARAMETER rootParameter[2];
    rootParameter[0].InitAsDescriptorTable(1, &table, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameter[1].InitAsConstants(1, 0);

    CD3DX12_ROOT_SIGNATURE_DESC rootScreenSigDesc(2, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[1];
    StaticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    rootScreenSigDesc.NumStaticSamplers = 1;
    rootScreenSigDesc.pStaticSamplers = StaticSamplers;

    CreateRootSignature(mDevice, rootScreenSigDesc, mScreenRootSignature.GetAddressOf());
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

    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = psoDesc;
    opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = psoDesc;
    skyPsoDesc.pRootSignature = mRootSignature.Get();
    skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    skyPsoDesc.VS = { reinterpret_cast<BYTE*>(mSkyVsByteCode->GetBufferPointer()), mSkyVsByteCode->GetBufferSize() };
    skyPsoDesc.PS = { reinterpret_cast<BYTE*>(mSkyPsByteCode->GetBufferPointer()), mSkyPsByteCode->GetBufferSize() };
    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC screenPsoDesc = psoDesc;
    screenPsoDesc.pRootSignature = mScreenRootSignature.Get();
    screenPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    screenPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    screenPsoDesc.VS = { reinterpret_cast<BYTE*>(mQuadVsByteCode->GetBufferPointer()), mQuadVsByteCode->GetBufferSize() };
    screenPsoDesc.PS = { reinterpret_cast<BYTE*>(mQuadPsByteCode->GetBufferPointer()), mQuadPsByteCode->GetBufferSize() };


    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState;
    ZeroMemory(&descPipelineState, sizeof(descPipelineState));
    
    descPipelineState.VS = { reinterpret_cast<BYTE*>(mQuadVsByteCode->GetBufferPointer()), mQuadVsByteCode->GetBufferSize() };
    descPipelineState.PS = { reinterpret_cast<BYTE*>(mQuadPsByteCode->GetBufferPointer()), mQuadPsByteCode->GetBufferSize() };
    descPipelineState.InputLayout = { mInputLayout.data(), static_cast<UINT>(mInputLayout.size()) };
    descPipelineState.pRootSignature = mScreenRootSignature.Get();
    descPipelineState.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    descPipelineState.DepthStencilState.DepthEnable = false;
    descPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    descPipelineState.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    descPipelineState.RasterizerState.DepthClipEnable = false;
    descPipelineState.SampleMask = UINT_MAX;
    descPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPipelineState.NumRenderTargets = 1;
    //descPipelineState.RTVFormats[0] = mRtvFormat[0];
    descPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    descPipelineState.SampleDesc.Count = 1;

    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&descPipelineState, IID_PPV_ARGS(&mPSOs["screen"])));

    D3D12_COMPUTE_PIPELINE_STATE_DESC horizontalBlurDesc;
    ZeroMemory(&horizontalBlurDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
    horizontalBlurDesc.pRootSignature = mPostProcessRootSignature.Get();
    horizontalBlurDesc.CS = {reinterpret_cast<BYTE*>(mcsHorizontalByteCode->GetBufferPointer()), mcsHorizontalByteCode->GetBufferSize()};
    horizontalBlurDesc.NodeMask = 0;
    horizontalBlurDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(mDevice->CreateComputePipelineState(&horizontalBlurDesc, IID_PPV_ARGS(&mPSOs["horzBlur"])));
    
    D3D12_COMPUTE_PIPELINE_STATE_DESC verticalBlurDesc = {};
    ZeroMemory(&verticalBlurDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
    verticalBlurDesc.pRootSignature = mPostProcessRootSignature.Get();
    verticalBlurDesc.CS = { reinterpret_cast<BYTE*>(mcsVerticalByteCode->GetBufferPointer()), mcsVerticalByteCode->GetBufferSize() };
    verticalBlurDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(mDevice->CreateComputePipelineState(&verticalBlurDesc, IID_PPV_ARGS(&mPSOs["vertBlur"])));
}

void D3DAppHelloWorld::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(mDevice.Get(), 1, mAllItems.size(), mMaterials.size(), mClientWidth, mClientHeight, mBackBufferFormat, mSRBuffer.get(), mRTBuffer.get()));
    }
}

void D3DAppHelloWorld::BuildShapeGeometry()
{
    GeometryGenerator geoGen;
    //auto totalVertexCount = circle.Vertices.size() + grid.Vertices.size() + lightBox.Vertices.size();
    
    std::unordered_map<std::string, GeometryGenerator::MeshData> meshes = {
        {"sphere", geoGen.CreateSphere(1, 500, 100) }
       ,{"grid", geoGen.CreateGrid(20.0f, 30.0f, 60, 40)}
       ,{"lightBox", geoGen.CreateBox(5.0f, 5.0f, 5.0f, 1)}
       ,{"screenQuad",geoGen.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f) }
    };
        
    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "shapeGeo";
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    UINT vertexOffset = 0;
    UINT indexOffset = 0;
    for (auto& e : meshes)
    {
        auto& mesh = e.second;

        SubmeshGeometry submesh;
        submesh.IndexCount = mesh.Indices32.size();
        submesh.BaseVertexLocation = vertexOffset;
        submesh.StartIndexLocation = indexOffset;

        geo->DrawArgs[e.first] = submesh;

        vertexOffset += e.second.Vertices.size();
        indexOffset += e.second.Indices32.size();

        for (size_t i = 0; i < mesh.Vertices.size(); ++i)
        {
            Vertex v;
            v.Pos = mesh.Vertices[i].Position;
            v.Tangent = mesh.Vertices[i].TangentU;
            v.Normal = mesh.Vertices[i].Normal;
            v.Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
            v.Tex0 = mesh.Vertices[i].TexC;

            vertices.push_back(v);
        }

        indices.insert(indices.end(), std::begin(mesh.GetIndices16()), std::end(mesh.GetIndices16()));
    }

    const UINT vbByteSize = vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = indices.size() * sizeof(uint16_t);

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = CreateDefaultBuffer(vertices.data(), vbByteSize, geo->VertexBufferUploader);
    geo->IndexBufferGPU = CreateDefaultBuffer(indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    mGeometries[geo->Name] = std::move(geo);
}

void D3DAppHelloWorld::BuildRenderItems()
{
    UINT objIndex = 0;
    std::unordered_map<std::string, std::string> geoMat = 
    {
        {"sphere", "sphere"}
       ,{"grid", "floor"}
       ,{"lightBox", "floor"}
    };

    for (auto& g : mGeometries["shapeGeo"]->DrawArgs)
    {
        if (g.first == "screenQuad") continue;
        auto ri = std::make_unique<RenderItem>();;
        ri->ObjCBIndex = objIndex++;
        ri->Geo = mGeometries["shapeGeo"].get();
        ri->Mat = mMaterials[geoMat[g.first]].get();
        ri->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        ri->IndexCount = g.second.IndexCount;
        ri->StartIndexLocation = g.second.StartIndexLocation;
        ri->BaseVertexLocation = g.second.BaseVertexLocation;

        if (g.first == "sphere")
        {
            XMStoreFloat4x4(&ri->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 2.0f, 3.0f));
        }
        else if (g.first == "lightBox")
        {
            XMFLOAT3 lightPos;
            XMStoreFloat3(&lightPos, MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi));
            XMStoreFloat4x4(&ri->World, XMMatrixScaling(0.04f, 0.04f, 0.04f) * XMMatrixTranslation(lightPos.x, lightPos.y * 7.0f, lightPos.z));
            mLightBox = ri.get();
        }
        else
        {
            ri->World = MathHelper::Identity4x4();
        }

        mLayerRItems[(int)RenderLayer::Opaque].push_back(ri.get());
        mAllItems.push_back(std::move(ri));
    }
    
    auto skyRItem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&skyRItem->World, XMMatrixScaling(50.0f, 50.0f, 50.0f));
    
    skyRItem->ObjCBIndex = objIndex++;
    skyRItem->Geo = mGeometries["shapeGeo"].get();
    skyRItem->Mat = mMaterials["sky"].get();
    skyRItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    skyRItem->IndexCount = skyRItem->Geo->DrawArgs["sphere"].IndexCount;
    skyRItem->StartIndexLocation = skyRItem->Geo->DrawArgs["sphere"].StartIndexLocation;
    skyRItem->BaseVertexLocation = skyRItem->Geo->DrawArgs["sphere"].BaseVertexLocation;

    mLayerRItems[(int)RenderLayer::Sky].push_back(skyRItem.get());
    mAllItems.push_back(std::move(skyRItem));

    auto screenQuad = std::make_unique<RenderItem>();
    screenQuad->World = MathHelper::Identity4x4();;

    screenQuad->ObjCBIndex = objIndex++;
    screenQuad->Geo = mGeometries["shapeGeo"].get();
    screenQuad->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    screenQuad->Mat = nullptr;
    screenQuad->IndexCount = screenQuad->Geo->DrawArgs["screenQuad"].IndexCount;
    screenQuad->StartIndexLocation = screenQuad->Geo->DrawArgs["screenQuad"].StartIndexLocation;
    screenQuad->BaseVertexLocation = screenQuad->Geo->DrawArgs["screenQuad"].BaseVertexLocation;

    mLayerRItems[(int)RenderLayer::PostProcessing].push_back(screenQuad.get());
    mAllItems.push_back(std::move(screenQuad));
}

void D3DAppHelloWorld::BuildMaterials()
{
    auto floor = std::make_unique<Material>();
    floor->Name = "floor";
    floor->NumFramesDirty = gNumFrameResources;
    floor->MatCBIndex = 1;
    floor->DiffuseAlbedo = XMFLOAT4(1.0f, 0.6f, 0.6f, 1.0f);
    floor->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    floor->Roughness = 0.05f;
    floor->DiffuseSrvHeapIndex = mTextures[2]->ID;

    auto sphere = std::make_unique<Material>();
    sphere->Name = "spheres";
    sphere->NumFramesDirty = gNumFrameResources;
    sphere->MatCBIndex = 0;
    sphere->DiffuseAlbedo = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    sphere->FresnelR0 = XMFLOAT3(1.0f, 1.0f, 1.0f);
    sphere->Roughness = 0.05f;
    sphere->DiffuseSrvHeapIndex = mTextures[0]->ID;
    sphere->hasNormalMap = 1;

    auto sky = std::make_unique<Material>();
    sky->Name = "sky";
    sky->NumFramesDirty = gNumFrameResources;
    sky->MatCBIndex = 2;
    sky->DiffuseAlbedo = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    sky->FresnelR0 = XMFLOAT3(0.95f, 0.93f, 0.88f);
    sky->Roughness = 0.05f;
    sky->DiffuseSrvHeapIndex = mTextures[3]->ID;
    
    mMaterials["sphere"] = std::move(sphere);
    mMaterials["floor"] = std::move(floor);
    mMaterials["sky"] = std::move(sky);
}

void D3DAppHelloWorld::BuildTextures()
{
    std::vector<std::wstring> texPaths = { 
        L"Texture/pbr/beaten-up-metal1-albedo.dds",
        L"Texture/pbr/beaten-up-metal1-Normal-dx.dds",
        L"Texture/tile.dds",
        L"Texture/grasscube1024.dds"
    };

    for (int i = 0; i < texPaths.size(); ++i)
    {
        auto& texPath = texPaths[i];
        mTextures[i] = std::make_unique<Texture>();
        mTextures[i]->ID = i;
        ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mDevice.Get(), mCommandList.Get(), texPath.c_str(), mTextures[i]->Resource, mTextures[i]->UploadHeap));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        srvDesc.Format = mTextures[i]->Resource->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = mTextures[i]->Resource->GetDesc().MipLevels;

        if (i == texPaths.size() - 1)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        }
        mTextures[i]->handle = mSRBuffer->GpuHandle();
        mDevice->CreateShaderResourceView(mTextures[i]->Resource.Get(), &srvDesc, mSRBuffer->CpuHandle());
    }
}

void D3DAppHelloWorld::UpdateMaterialCB()
{
    auto currMaterialCB = mCurrentFrameResource->MaterialCB.get();
    for (auto& e : mMaterials)
    {
        Material* mat = e.second.get();
        if (mat->NumFramesDirty > 0)
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

void D3DAppHelloWorld::UpdateObjectCBs()
{
    auto currObjectCB = mCurrentFrameResource->ObjectCB.get();
    for (auto& item : mAllItems)
    {
        if (item->NumFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&item->World);
            ObjectConstants objConstants;
            XMStoreFloat4x4(&objConstants.World, world);
            objConstants.MaterialIndex = item->Mat ? item->Mat->MatCBIndex : 0;
            currObjectCB->CopyData(item->ObjCBIndex, objConstants);
            item->NumFramesDirty--;
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
    XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

    XMFLOAT3 lightPos;
    XMStoreFloat3(&lightPos, MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi));
    XMStoreFloat4x4(&mLightBox->World, XMMatrixScaling(0.04f, 0.04f, 0.04f) * XMMatrixTranslation(lightPos.x, lightPos.y * 7.0f, lightPos.z));
    mLightBox->NumFramesDirty = 3;
    XMStoreFloat4x4(&mMainPassCB.View, view);
    XMStoreFloat4x4(&mMainPassCB.InvView, invView);
    XMStoreFloat4x4(&mMainPassCB.Proj, proj);
    XMStoreFloat4x4(&mMainPassCB.InvProj, invProj);
    XMStoreFloat4x4(&mMainPassCB.ViewProj, viewProj);
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, invViewProj);
    XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);

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

void D3DAppHelloWorld::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems)
{
    UINT objCBByteSize = (sizeof(ObjectConstants) + 255) & ~255;
    auto objectCB = mCurrentFrameResource->ObjectCB->Resource();
    
    for (auto i = 0; i < rItems.size(); ++i)
    {
        auto ri = rItems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        auto cbvIndex = ri->ObjCBIndex;
        auto cbAddress = objectCB->GetGPUVirtualAddress();
        cbAddress += (cbvIndex * objCBByteSize);

        cmdList->SetGraphicsRootConstantBufferView(0, cbAddress);
        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
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

    UpdateObjectCBs();
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
        ImGui::Checkbox("Blur", &mIsBlurEnabled);      // Edit bools storing our window open/close state
        ImGui::SliderInt("Blur Passes", &blurPasses, 1, 20);
        if (ImGui::SliderFloat("Reflection", &reflective, 0.0f, 1.0f))
        {
            mMaterials["sphere"]->FresnelR0.x = reflective;
            mMaterials["sphere"]->FresnelR0.y = reflective;
            mMaterials["sphere"]->FresnelR0.z = reflective;
            mMaterials["sphere"]->NumFramesDirty = 3;
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


    D3D12_VIEWPORT screenViewport = { 0.0f, 0.0f, static_cast<FLOAT>(mClientWidth), static_cast<FLOAT>(mClientHeight), 0.0f, 1.0f };
    mCommandList->RSSetViewports(1, &screenViewport);

    RECT scissorRect = { 0L, 0L, static_cast<LONG>(mClientWidth), static_cast<LONG>(mClientHeight) };
    mCommandList->RSSetScissorRects(1, &scissorRect);

    auto currentBackBuffer = mSwapChainBuffers[mCurrentBufferIndex].Get();
    auto renderTargetView = mRTHandles[mCurrentBufferIndex];
    auto depthStencilView = mDsvHeap->GetCPUDescriptorHandleForHeapStart();

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCurrentFrameResource->ScreenMap.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    mCommandList->ClearRenderTargetView(mCurrentFrameResource->ScreenCpuRtv, clearColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    mCommandList->OMSetRenderTargets(1, &mCurrentFrameResource->ScreenCpuRtv, true, &depthStencilView);

    ID3D12DescriptorHeap* descriptorHeaps[] = { mSRBuffer->DescriptorHeap() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    //bind all materials
    auto matCB = mCurrentFrameResource->MaterialCB->Resource();
    mCommandList->SetGraphicsRootShaderResourceView(2, matCB->GetGPUVirtualAddress());

    mCommandList->SetGraphicsRootDescriptorTable(3, mTextures[3]->handle);
  
    //bind all textures
    mCommandList->SetGraphicsRootDescriptorTable(4, mTextures[0]->handle);

    auto passCBVIndex = mPassCbvOffset + mCurrentFrameResourceIndex;
    auto passCB = mCurrentFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    mCommandList->SetPipelineState(mPSOs["opaque"].Get());
    DrawRenderItems(mCommandList.Get(), mLayerRItems[(int)RenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs["sky"].Get());
    DrawRenderItems(mCommandList.Get(), mLayerRItems[(int)RenderLayer::Sky]);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCurrentFrameResource->ScreenMap.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
    
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    mCommandList->ClearRenderTargetView(renderTargetView, clearColor, 0, nullptr);

    mCommandList->OMSetRenderTargets(1, &renderTargetView, true, nullptr);
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    mCommandList->SetGraphicsRootSignature(mScreenRootSignature.Get());
    mCommandList->SetPipelineState(mPSOs["screen"].Get());
    mCommandList->SetGraphicsRootDescriptorTable(0, mCurrentFrameResource->ScreenGpuSrv);
    float test = 0.2f;
    mCommandList->SetGraphicsRoot32BitConstants(1, 1, &test, 0);
    DrawRenderItems(mCommandList.Get(), mLayerRItems[(int)RenderLayer::PostProcessing]);

    if (mIsBlurEnabled)
    {
        mBlurFilter->Execute(mCommandList.Get(), mPostProcessRootSignature.Get(),
                             mPSOs["horzBlur"].Get(), mPSOs["vertBlur"].Get(), currentBackBuffer, blurPasses);

        //// Transition to PRESENT state.
        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));
    }
    else
    {

        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

    

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* commandLists[] = { mCommandList.Get() };

    mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    mSwapChain->Present(0, mAllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0);
    ///////////////////////////////////////////////////////////////////////////////
    mCurrentFrameResource->Fence = ++mFenceValue;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));
    mCurrentBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    ////////////////////////////////////////////////////////////////////////////////
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
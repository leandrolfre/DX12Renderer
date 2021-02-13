#include "Mesh.h"
#include "ResourceManager.h"
#include "GeometryGenerator.h"
#include <d3dcompiler.h>

Mesh::Mesh(MeshData& meshData, Material* material)
{
    UINT vertexOffset = 0;
    UINT indexOffset = 0;

    SubmeshGeometry submesh;
    submesh.IndexCount = meshData.Indices32.size();
    submesh.BaseVertexLocation = vertexOffset;
    submesh.StartIndexLocation = indexOffset;
    submesh.Mat = material;
    mSubmeshes.emplace_back(submesh);

    for (size_t i = 0; i < meshData.Vertices.size(); ++i)
    {
        Vertex v;
        v.Pos = meshData.Vertices[i].Position;
        v.Normal = meshData.Vertices[i].Normal;
        v.Tangent = meshData.Vertices[i].TangentU;
        v.Tex0 = meshData.Vertices[i].TexC;
        mVertices.emplace_back(v);
    }

    mIndices.insert(mIndices.end(), std::cbegin(meshData.GetIndices16()), std::cend(meshData.GetIndices16()));
}

void Mesh::UploadData(ID3D12GraphicsCommandList* cmdList)
{
    const UINT vbByteSize = mVertices.size() * sizeof(Vertex);
    const UINT ibByteSize = mIndices.size() * sizeof(uint16_t);

    mVertexByteStride = sizeof(Vertex);
    mVertexBufferByteSize = vbByteSize;
    mIndexFormat = DXGI_FORMAT_R16_UINT;
    mIndexBufferByteSize = ibByteSize;

    mVertexBufferGPU = ResourceManager::Get().CreateDefaultBuffer(mVertices.data(), vbByteSize, mVertexBufferUploader, cmdList);
    mIndexBufferGPU = ResourceManager::Get().CreateDefaultBuffer(mIndices.data(), ibByteSize, mIndexBufferUploader, cmdList);

    mVertexBufferView.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
    mVertexBufferView.SizeInBytes = mVertexBufferByteSize;
    mVertexBufferView.StrideInBytes = mVertexByteStride;

    mIndexBufferView.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
    mIndexBufferView.SizeInBytes = mIndexBufferByteSize;
    mIndexBufferView.Format = mIndexFormat;

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mVertexBufferCPU));
    CopyMemory(mVertexBufferCPU->GetBufferPointer(), mVertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mIndexBufferCPU));
    CopyMemory(mIndexBufferCPU->GetBufferPointer(), mIndices.data(), ibByteSize);
}

void Mesh::Draw(ID3D12GraphicsCommandList* cmdList, UploadBuffer<ObjectConstants>* objectCB)
{
	cmdList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	cmdList->IASetIndexBuffer(&mIndexBufferView);
    cmdList->IASetPrimitiveTopology(mPrimitiveType);
    
    XMMATRIX world = XMLoadFloat4x4(&World);
    ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.World, world);

	for (auto& submesh : mSubmeshes)
	{   
        if (submesh.Mat)
        {
            objConstants.MaterialIndex = submesh.Mat->MatCBIndex;
            cmdList->SetGraphicsRootDescriptorTable(1, submesh.Mat->TextureHandle);
        }
        
        auto bufferLocation = objectCB->CopyData(objConstants);
        cmdList->SetGraphicsRootConstantBufferView(0, bufferLocation);
   		cmdList->DrawIndexedInstanced(submesh.IndexCount, 1, submesh.StartIndexLocation, submesh.BaseVertexLocation, 0);
	}
}

void Mesh::DisposeUploaders()
{
	mVertexBufferUploader = nullptr;
	mIndexBufferUploader = nullptr;
}

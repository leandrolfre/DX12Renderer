#pragma once

#include <d3d12.h>
#include <vector>
#include <memory>

class Mesh;
class PostProcessing;
class FrameResource;
struct ID3D12GraphicsCommandList;

class PostProcessingPipeline
{
public:
    PostProcessingPipeline(ID3D12PipelineState* screenPso) : mScreenPso(screenPso)
    {}
    ~PostProcessingPipeline() = default;
    PostProcessingPipeline(const PostProcessingPipeline&) = delete;
    PostProcessingPipeline& operator=(const PostProcessingPipeline&) = delete;
    void Init();
    void Run(ID3D12GraphicsCommandList* CmdList, FrameResource* frameResource);
    inline void AddPostProcessing(std::unique_ptr < PostProcessing> postProcessing) { mPostprocessingList.push_back(std::move(postProcessing)); }
private:
    std::unique_ptr<Mesh>  mScreenQuad;
    ID3D12PipelineState* mScreenPso;
    std::vector<std::unique_ptr<PostProcessing>> mPostprocessingList;
};


#pragma once

#include <RendererConstants.h>
#include <Scene.h>
#include <Fence.h>
#include <resource/Resource.h>

class Renderer;

class ComputePass
{
public:
    ComputePass( wchar_t const* name,
                 wchar_t const* shaderFilename,
                 UINT threadGroupCountX,
                 UINT threadGroupCountY,
                 UINT threadGroupCountZ );
    ~ComputePass();

    double getExecutionTimeMilliseconds() const { return m_executionTimeInMilliseconds; }

    void record( Renderer& renderer, ComPtr<ID3D12GraphicsCommandList> commandList );

    void addFenceToSignal(Fence& fence);
    std::vector<Fence*> const& getFencesToSignal() const { return m_fencesToSignal; }
    void addFenceToWaitOn(Fence& fence);
    std::vector<Fence*> const& getFencesToWaitOn() const { return m_fencesToWaitOn; }

    void addResourceView( Descriptor const descriptor );
    void setResourceView( UINT index, Descriptor const descriptor );
    void setThreadGroupCountX( UINT threadGroupCountX ) { m_threadGroupCountX = threadGroupCountX; }
    void setThreadGroupCountY( UINT threadGroupCountY ) { m_threadGroupCountY = threadGroupCountY; }
    void setThreadGroupCountZ( UINT threadGroupCountZ ) { m_threadGroupCountZ = threadGroupCountZ; }
    void setThreadGroupCounts( UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ );
    void transitionResourcesForNextFrame( ComPtr<ID3D12GraphicsCommandList> commandList );

private:
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE s_rootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS m_computeShader;
    };

    std::wstring m_name;
    std::wstring m_shaderFilename;
    UINT m_threadGroupCountX;
    UINT m_threadGroupCountY;
    UINT m_threadGroupCountZ;

    ComPtr<ID3D12PipelineState> m_pso;

    uint64_t m_profilerQueryIndex;
    double m_executionTimeInMilliseconds;

    ResourceHandle m_passBuffer;
    std::vector<UINT> m_passBufferData;
    Descriptor m_passBufferDescriptor;

    std::vector<Descriptor> m_resourceViews;

    std::vector<Fence*> m_fencesToSignal;
    std::vector<Fence*> m_fencesToWaitOn;
};


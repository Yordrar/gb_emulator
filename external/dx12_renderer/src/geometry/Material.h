#pragma once

#include <d3d12.h>
#include <d3dx12/d3dx12.h>
#include <dxgi1_6.h>

#include <unordered_map>

#include <resource/Descriptor.h>

class Resource;

class Material
{
public:
    struct Technique
    {
        std::wstring m_name;
        std::wstring m_vertexShaderFilename;
        std::wstring m_pixelShaderFilename;
        CD3DX12_BLEND_DESC m_blendState = CD3DX12_BLEND_DESC( CD3DX12_DEFAULT{} );
        CD3DX12_RASTERIZER_DESC m_rasterizerState = CD3DX12_RASTERIZER_DESC( CD3DX12_DEFAULT{} );
        CD3DX12_DEPTH_STENCIL_DESC m_depthStencilState = CD3DX12_DEPTH_STENCIL_DESC( CD3DX12_DEFAULT{} );
        D3D12_PRIMITIVE_TOPOLOGY_TYPE m_topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        CD3DX12_RT_FORMAT_ARRAY m_rtFormats;
        DXGI_FORMAT m_dsFormat;
    };

    struct MaterialDesc
    {
        std::wstring m_name;
        std::vector<Technique> m_techniques;
        std::vector<Descriptor> m_resourceViews;
        std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
    };

    Material( MaterialDesc const& materialDesc );
    ~Material() = default;

    Material( const Material& ) = delete;
    Material& operator= ( const Material& ) = delete;

    std::wstring const& getName() const { return m_desc.m_name; }
    ComPtr<ID3D12PipelineState> getPSOForTechnique( wchar_t const* techniqueName ) const;
    ResourceHandle getMaterialBufferResource() const { return m_materialBuffer; }
    Descriptor getMaterialBufferDescriptor() const { return m_materialBufferDescriptor; }
    std::vector<Descriptor> const& getResourceViews() const { return m_desc.m_resourceViews; }

    bool hasTechnique( wchar_t const* techniqueName ) const;

private:
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE s_rootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_VS m_vertexShader;
        CD3DX12_PIPELINE_STATE_STREAM_PS m_pixelShader;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC m_blendState;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER m_rasterizerState;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL m_depthStencilState;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT m_inputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY m_topologyType;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS m_rtFormats;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT m_dsFormat;
    };

    MaterialDesc m_desc;

    ResourceHandle m_materialBuffer;
    std::vector<UINT> m_bindlessIndices;
    Descriptor m_materialBufferDescriptor;
    
    // PSO name is: <technique_name>
    using PSOCache = std::unordered_map< std::wstring, ComPtr<ID3D12PipelineState> >;
    PSOCache m_psoCache;
};
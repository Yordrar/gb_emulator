#include "Material.h"

#include <Renderer.h>
#include <geometry/ShaderManager.h>
#include <resource/ResourceManager.h>

Material::Material( MaterialDesc const& materialDesc )
    : m_desc( materialDesc )
{
    for ( Technique const& technique : m_desc.m_techniques )
    {
        std::wstring passNameDefine;
        passNameDefine.resize( technique.m_name.size() );
        std::transform( technique.m_name.begin(), technique.m_name.end(), passNameDefine.begin(), std::towupper );
        ShaderManager::ShaderDesc vertexShaderDesc =
        {
            .m_filename = technique.m_vertexShaderFilename,
            .m_entryPoint = technique.m_name + L"_vs",
            .m_shaderType = ShaderManager::ShaderType::VertexShader,
            .m_enableDebug = true,
            .m_defines = {passNameDefine},
        };

        ShaderManager::ShaderDesc pixelShaderDesc =
        {
            .m_filename = technique.m_pixelShaderFilename,
            .m_entryPoint = technique.m_name + L"_ps",
            .m_shaderType = ShaderManager::ShaderType::PixelShader,
            .m_enableDebug = true,
            .m_defines = {passNameDefine},
        };

        PipelineStateStream pipelineStateStream =
        {
            .s_rootSignature = Renderer::getRootSignature().Get(),
            .m_vertexShader = ShaderManager::it().getShader( vertexShaderDesc ),
            .m_pixelShader = ShaderManager::it().getShader( pixelShaderDesc ),
            .m_blendState = technique.m_blendState,
            .m_rasterizerState = technique.m_rasterizerState,
            .m_depthStencilState = technique.m_depthStencilState,
            .m_inputLayout = D3D12_INPUT_LAYOUT_DESC{ m_desc.m_inputLayout.data(), static_cast<UINT>( m_desc.m_inputLayout.size() ) },
            .m_topologyType = technique.m_topologyType,
            .m_rtFormats = technique.m_rtFormats,
            .m_dsFormat = technique.m_dsFormat,
        };

        ComPtr<ID3D12PipelineState> pso;
        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
        {
            .SizeInBytes = sizeof( PipelineStateStream ),
            .pPipelineStateSubobjectStream = &pipelineStateStream,
        };
        Renderer::device()->CreatePipelineState( &pipelineStateStreamDesc, IID_PPV_ARGS( &pso ) );

        m_psoCache[ technique.m_name ] = pso;
        m_psoCache[ technique.m_name ]->SetName( ( m_desc.m_name + L"/" + technique.m_name ).c_str() );
    }

    for ( Descriptor const& resourceView : m_desc.m_resourceViews )
    {
        m_bindlessIndices.push_back( resourceView.getDescriptorIndex() );
    }
    m_materialBuffer = ResourceManager::it().createResource( ( m_desc.m_name + L"_bindlessIndicesBuffer" ).c_str(),
                                                                      CD3DX12_RESOURCE_DESC::Buffer( std::max( m_bindlessIndices.size() * sizeof( UINT ), 1Ui64 ) ),
                                                                      D3D12_SUBRESOURCE_DATA{ m_bindlessIndices.data(), static_cast<LONG_PTR>( m_bindlessIndices.size() * sizeof( UINT ) ), 0 } );
    
    D3D12_RESOURCE_DESC materialBufferResourceDesc = ResourceManager::it().getResourceDesc(m_materialBuffer);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
    {
        .Format = DXGI_FORMAT_R32_TYPELESS,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    };
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    srvDesc.Buffer.NumElements = static_cast<UINT>(std::max(m_bindlessIndices.size(), 1Ui64));
    srvDesc.Buffer.StructureByteStride = 0;
    m_materialBufferDescriptor = ResourceManager::it().getShaderResourceView(m_materialBuffer, srvDesc);
}

ComPtr<ID3D12PipelineState> Material::getPSOForTechnique( wchar_t const* techniqueName ) const
{
    PSOCache::const_iterator it = m_psoCache.find( techniqueName );
    if ( it != m_psoCache.end() )
    {
        return it->second;
    }

    return nullptr;
}

bool Material::hasTechnique( wchar_t const* techniqueName ) const
{
    for ( Technique const& technique : m_desc.m_techniques )
    {
        if ( technique.m_name == techniqueName )
        {
            return true;
        }
    }
    return false;
}

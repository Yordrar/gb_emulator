#pragma once

#include <Manager.h>

class ShaderManager : public Manager<ShaderManager>
{
    friend class Manager<ShaderManager>;
public:
    enum class ShaderType
    {
        VertexShader,
        PixelShader,
        ComputeShader
    };

    struct ShaderDesc
    {
        std::wstring m_filename;
        std::wstring m_entryPoint;
        ShaderType m_shaderType;
        bool m_enableDebug;
        std::vector<std::wstring> m_defines;

        bool operator==( ShaderDesc const& other ) const
        {
            return ( m_filename == other.m_filename &&
                     m_entryPoint == other.m_entryPoint &&
                     m_shaderType == other.m_shaderType &&
                     m_enableDebug == other.m_enableDebug &&
                     m_defines == other.m_defines );
        }

        struct Hasher
        {
            size_t operator()( ShaderDesc const& shaderDesc ) const noexcept
            {
                std::size_t hash = std::hash<std::wstring>{}( shaderDesc.m_filename );
                hash = hash ^ ( std::hash<std::wstring>{}( shaderDesc.m_entryPoint ) << 1 );
                hash = hash ^ ( std::hash<char>{}( static_cast<char>( shaderDesc.m_shaderType ) ) << 1 );
                hash = hash ^ ( std::hash<bool>{}( shaderDesc.m_enableDebug ) << 1 );
                for ( std::wstring const& define : shaderDesc.m_defines )
                {
                    hash = hash ^ ( std::hash<std::wstring>{}( define ) << 1 );
                }
                return hash;
            }
        };
    };

    ~ShaderManager() = default;

    D3D12_SHADER_BYTECODE getShader( ShaderDesc& params );

private:
    ShaderManager();

    LPCWSTR shaderTypeToTargetString( ShaderType type );

    using ShaderMap = std::unordered_map< ShaderDesc, ComPtr<IDxcBlob>, ShaderDesc::Hasher >;
    ShaderMap m_shaders;
    ComPtr<IDxcUtils> m_utils;
    ComPtr<IDxcCompiler3> m_compiler;
    ComPtr<IDxcIncludeHandler> m_includeHandler;
};

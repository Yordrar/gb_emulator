#include "ShaderManager.h"

#include <Utils.h>

ShaderManager::ShaderManager()
    : m_utils( nullptr )
    , m_compiler( nullptr )
    , m_includeHandler( nullptr )
{
    DxcCreateInstance( CLSID_DxcUtils, IID_PPV_ARGS( &m_utils ) );
    DxcCreateInstance( CLSID_DxcCompiler, IID_PPV_ARGS( &m_compiler ) );
    m_utils->CreateDefaultIncludeHandler( &m_includeHandler );
}

D3D12_SHADER_BYTECODE ShaderManager::getShader( ShaderDesc& shaderDesc )
{
    if (!shaderDesc.m_filename.c_str() || shaderDesc.m_filename == L"")
    {
        return D3D12_SHADER_BYTECODE{ nullptr, 0 };
    }

    ShaderMap::iterator it = m_shaders.find( shaderDesc );

    if ( it != m_shaders.end() )
    {
        CD3DX12_SHADER_BYTECODE shaderBytecode( it->second->GetBufferPointer(), it->second->GetBufferSize() );
        return shaderBytecode;
    }

    std::string filename = WideStrToStr(shaderDesc.m_filename);

    std::string csoFilename = std::regex_replace(filename, std::regex("hlsl"), "cso");
    std::wstring csoFilenameWideStr = std::wstring(csoFilename.begin(), csoFilename.end());

    std::string pdbFilename = std::regex_replace(filename, std::regex("hlsl"), "pdb");
    std::wstring pdbFilenameWideStr = std::wstring(pdbFilename.begin(), pdbFilename.end());

    std::vector<LPCWSTR> compileArgs;
    compileArgs.push_back( shaderDesc.m_filename.c_str() );
    compileArgs.push_back( L"-E" );
    compileArgs.push_back( shaderDesc.m_entryPoint.c_str() );
    compileArgs.push_back( L"-T" );
    compileArgs.push_back( shaderTypeToTargetString( shaderDesc.m_shaderType ) );
    if ( shaderDesc.m_enableDebug )
    {
        compileArgs.push_back( L"-Zi" );
    }
    if ( shaderDesc.m_defines.size() > 0 )
    {
        compileArgs.push_back( L"-D" );
        for ( std::wstring& define : shaderDesc.m_defines )
        {
            compileArgs.push_back( define.c_str() );
        }
    }
    compileArgs.push_back( L"-Fo" );
    compileArgs.push_back( csoFilenameWideStr.c_str() );
    compileArgs.push_back( L"-Fd" );
    compileArgs.push_back( pdbFilenameWideStr.c_str() );
    compileArgs.push_back( L"-HV 2021" );

    // Open source file.
    ComPtr<IDxcBlobEncoding> sourceBlob = nullptr;
    HRESULT result = m_utils->LoadFile( shaderDesc.m_filename.c_str(), nullptr, &sourceBlob );
    if ( result != S_OK )
    {
        return D3D12_SHADER_BYTECODE{ nullptr, 0 };
    }
    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
    sourceBuffer.Size = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.

    // Compile it with specified arguments.
    ComPtr<IDxcResult> compilationResult;
    m_compiler->Compile(
        &sourceBuffer,                             // Source buffer.
        compileArgs.data(),                        // Array of pointers to arguments.
        static_cast<UINT32>( compileArgs.size() ), // Number of arguments.
        m_includeHandler.Get(),                    // User-provided interface to handle #include directives (optional).
        IID_PPV_ARGS( &compilationResult )         // Compiler output status, buffer, and errors.
    );
    ComPtr<IDxcBlob> compiledBytecodeBlob;
    ComPtr<IDxcBlobUtf16> shaderName = nullptr;
    compilationResult->GetOutput( DXC_OUT_OBJECT, IID_PPV_ARGS( &compiledBytecodeBlob ), &shaderName );
   
    // Print errors if present
    ComPtr<IDxcBlobUtf8> pErrors = nullptr;
    compilationResult->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &pErrors ), nullptr );
    // Note that d3dcompiler would return null if no errors or warnings are present.
    // IDxcCompiler3::Compile will always return an error buffer, but its length
    // will be zero if there are no warnings or errors.
    if ( pErrors != nullptr && pErrors->GetStringLength() != 0 )
    {
        OutputDebugStringA( pErrors->GetStringPointer() );
        DebugBreak();
    }

    // Save pdb.
    ComPtr<IDxcBlob> pdbBlob = nullptr;
    ComPtr<IDxcBlobUtf16> pdbName = nullptr;
    HRESULT hr = compilationResult->GetOutput( DXC_OUT_PDB, IID_PPV_ARGS( &pdbBlob ), &pdbName );
    if ( pdbBlob && pdbName )
    {
        FILE* fp = NULL;

        // Note that if you don't specify -Fd, a pdb name will be automatically generated. Use this file name to save the pdb so that PIX can find it quickly.
        _wfopen_s( &fp, pdbName->GetStringPointer(), L"wb" );
        if ( fp )
        {
            fwrite( pdbBlob->GetBufferPointer(), pdbBlob->GetBufferSize(), 1, fp );
            fclose( fp );
        }
    }

    m_shaders[ shaderDesc ] = compiledBytecodeBlob;
    CD3DX12_SHADER_BYTECODE shaderBytecode( m_shaders[ shaderDesc ]->GetBufferPointer(), m_shaders[ shaderDesc ]->GetBufferSize() );
    return shaderBytecode;
}

LPCWSTR ShaderManager::shaderTypeToTargetString( ShaderType type )
{
    switch ( type )
    {
        case ShaderType::VertexShader:
            return L"vs_6_6";
        case ShaderType::PixelShader:
            return L"ps_6_6";
        case ShaderType::ComputeShader:
            return L"cs_6_6";
        default:
            assert(false);
            return L"";
    }
}
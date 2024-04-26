#include <Utils.h>

std::string WideStrToStr( const std::wstring& wstr )
{
    if ( wstr.empty() ) return std::string();
    int size_needed = WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], (int)wstr.size(), NULL, 0, NULL, NULL );
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], (int)wstr.size(), &strTo[ 0 ], size_needed, NULL, NULL );
    return strTo;
}

std::wstring StrToWideStr( const std::string& str )
{
    if ( str.empty() ) return std::wstring();
    int size_needed = MultiByteToWideChar( CP_UTF8, 0, &str[ 0 ], (int)str.size(), NULL, 0 );
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar( CP_UTF8, 0, &str[ 0 ], (int)str.size(), &wstrTo[ 0 ], size_needed );
    return wstrTo;
}

D3D12_SRV_DIMENSION resourceDimensionToSRVDimension(D3D12_RESOURCE_DESC const& resourceDesc)
{
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
        assert(false);
        return D3D12_SRV_DIMENSION_UNKNOWN;
        break;
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        return D3D12_SRV_DIMENSION_BUFFER;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        return resourceDesc.DepthOrArraySize == 1 ? D3D12_SRV_DIMENSION_TEXTURE1D : D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        return resourceDesc.DepthOrArraySize == 1 ? D3D12_SRV_DIMENSION_TEXTURE2D : D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        return D3D12_SRV_DIMENSION_TEXTURE3D;
        break;
    }
    assert(false);
    return D3D12_SRV_DIMENSION_UNKNOWN;
}

D3D12_UAV_DIMENSION resourceDimensionToUAVDimension(D3D12_RESOURCE_DESC const& resourceDesc)
{
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
        assert(false);
        return D3D12_UAV_DIMENSION_UNKNOWN;
        break;
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        return D3D12_UAV_DIMENSION_BUFFER;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        return resourceDesc.DepthOrArraySize == 1 ? D3D12_UAV_DIMENSION_TEXTURE1D : D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        return resourceDesc.DepthOrArraySize == 1 ? D3D12_UAV_DIMENSION_TEXTURE2D : D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        return D3D12_UAV_DIMENSION_TEXTURE3D;
        break;
    }
    assert(false);
    return D3D12_UAV_DIMENSION_UNKNOWN;
}

D3D12_RTV_DIMENSION resourceDimensionToRTVDimension(D3D12_RESOURCE_DESC const& resourceDesc)
{
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
        assert(false);
        return D3D12_RTV_DIMENSION_UNKNOWN;
        break;
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        return D3D12_RTV_DIMENSION_BUFFER;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        return resourceDesc.DepthOrArraySize == 1 ? D3D12_RTV_DIMENSION_TEXTURE1D : D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        return resourceDesc.DepthOrArraySize == 1 ? D3D12_RTV_DIMENSION_TEXTURE2D : D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        return D3D12_RTV_DIMENSION_TEXTURE3D;
        break;
    }
    assert(false);
    return D3D12_RTV_DIMENSION_UNKNOWN;
}

D3D12_DSV_DIMENSION resourceDimensionToDSVDimension(D3D12_RESOURCE_DESC const& resourceDesc)
{
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    case D3D12_RESOURCE_DIMENSION_BUFFER:
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        assert(false);
        return D3D12_DSV_DIMENSION_UNKNOWN;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        return resourceDesc.DepthOrArraySize == 1 ? D3D12_DSV_DIMENSION_TEXTURE1D : D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        return resourceDesc.DepthOrArraySize == 1 ? D3D12_DSV_DIMENSION_TEXTURE2D : D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        break;
    }
    assert(false);
    return D3D12_DSV_DIMENSION_UNKNOWN;
}

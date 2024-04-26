#pragma once

std::string WideStrToStr( const std::wstring& wstr );
std::wstring StrToWideStr( const std::string& str );

D3D12_SRV_DIMENSION resourceDimensionToSRVDimension(D3D12_RESOURCE_DESC const& resourceDesc);
D3D12_UAV_DIMENSION resourceDimensionToUAVDimension(D3D12_RESOURCE_DESC const& resourceDesc);
D3D12_RTV_DIMENSION resourceDimensionToRTVDimension(D3D12_RESOURCE_DESC const& resourceDesc);
D3D12_DSV_DIMENSION resourceDimensionToDSVDimension(D3D12_RESOURCE_DESC const& resourceDesc);
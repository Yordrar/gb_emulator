#include <resource/ResourceHandle.h>
#include <resource/ResourceManager.h>
#include <Utils.h>

bool ResourceHandle::isValid() const
{
    return m_index != RESOURCE_HANDLE_INVALID_INDEX &&
        m_generation != RESOURCE_HANDLE_INVALID_GENERATION &&
        ResourceManager::it().doesResourceHandlePointToValidResource(*this);
}

Descriptor const& ResourceHandle::getDefaultConstantBufferView()
{
    auto cbvDesc = getDefaultCBVDesc();
    return getConstantBufferView(cbvDesc);
}

Descriptor const& ResourceHandle::getDefaultShaderResourceView()
{
    return getShaderResourceView(getDefaultSRVDesc());
}

Descriptor const& ResourceHandle::getDefaultUnorderedAccessView()
{
    return getUnorderedAccessView(getDefaultUAVDesc());
}

Descriptor const& ResourceHandle::getDefaultRenderTargetView()
{
    return getRenderTargetView(getDefaultRTVDesc());
}

Descriptor const& ResourceHandle::getDefaultDepthStencilView(D3D12_DSV_FLAGS flags)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = getDefaultDSVDesc();
    dsvDesc.Flags = flags;
    return getDepthStencilView(dsvDesc);
}

Descriptor const& ResourceHandle::getShaderResourceView(UINT mostDetailedMip, UINT numMips)
{
    D3D12_RESOURCE_DESC resourceDesc = ResourceManager::it().getResourceDesc(*this);

    if (numMips == 0)
    {
        numMips = resourceDesc.MipLevels - mostDetailedMip;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = getDefaultSRVDesc();
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        assert(false);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            srvDesc.Texture1D.MostDetailedMip = mostDetailedMip;
            srvDesc.Texture1D.MipLevels = numMips;
        }
        else
        {
            srvDesc.Texture1DArray.MostDetailedMip = mostDetailedMip;
            srvDesc.Texture1DArray.MipLevels = numMips;
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;
            srvDesc.Texture2D.MipLevels = numMips;
        }
        else
        {
            srvDesc.Texture2DArray.MostDetailedMip = mostDetailedMip;
            srvDesc.Texture2DArray.MipLevels = numMips;
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        srvDesc.Texture3D.MostDetailedMip = mostDetailedMip;
        srvDesc.Texture3D.MipLevels = numMips;
        break;
    }
    return getShaderResourceView(srvDesc);
}

Descriptor const& ResourceHandle::getUnorderedAccessView(UINT mipSlice)
{
    D3D12_RESOURCE_DESC resourceDesc = ResourceManager::it().getResourceDesc(*this);
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = getDefaultUAVDesc();
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        assert(false);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            uavDesc.Texture1D.MipSlice = mipSlice;
        }
        else
        {
            uavDesc.Texture1DArray.MipSlice = mipSlice;
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            uavDesc.Texture2D.MipSlice = mipSlice;
        }
        else
        {
            uavDesc.Texture2DArray.MipSlice = mipSlice;
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        uavDesc.Texture3D.MipSlice = mipSlice;
        break;
    }
    return getUnorderedAccessView(uavDesc);
}

Descriptor const& ResourceHandle::getConstantBufferView(D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc)
{
    return ResourceManager::it().getConstantBufferView(*this, cbvDesc);
}

Descriptor const& ResourceHandle::getShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC const& srvDesc)
{
    return ResourceManager::it().getShaderResourceView(*this, srvDesc);
}

Descriptor const& ResourceHandle::getUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const& uavDesc)
{
    return ResourceManager::it().getUnorderedAccessView(*this, uavDesc);
}

Descriptor const& ResourceHandle::getRenderTargetView(D3D12_RENDER_TARGET_VIEW_DESC const& rtvDesc)
{
    return ResourceManager::it().getRenderTargetView(*this, rtvDesc);
}

Descriptor const& ResourceHandle::getDepthStencilView(D3D12_DEPTH_STENCIL_VIEW_DESC const& dsvDesc)
{
    return ResourceManager::it().getDepthStencilView(*this, dsvDesc);
}

void ResourceHandle::setNeedsCopyToGPU()
{
    ResourceManager::it().setResourceNeedsCopyToGPU(*this);
}

D3D12_CONSTANT_BUFFER_VIEW_DESC ResourceHandle::getDefaultCBVDesc()
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc =
    {
        .BufferLocation = ResourceManager::it().getD3DResource(*this)->GetGPUVirtualAddress(),
        .SizeInBytes = (UINT)Resource::getSizeAligned256(ResourceManager::it().getResourceDesc(*this).Width),
    };
    return cbvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC ResourceHandle::getDefaultSRVDesc()
{
    D3D12_RESOURCE_DESC resourceDesc = ResourceManager::it().getResourceDesc(*this);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
    {
        .Format = resourceDesc.Format,
        .ViewDimension = resourceDimensionToSRVDimension(resourceDesc),
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    };
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        assert(false);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            srvDesc.Texture1D =
            {
                .MostDetailedMip = 0,
                .MipLevels = resourceDesc.MipLevels,
                .ResourceMinLODClamp = 0.0f,
            };
        }
        else
        {
            srvDesc.Texture1DArray =
            {
                .MostDetailedMip = 0,
                .MipLevels = resourceDesc.MipLevels,
                .FirstArraySlice = 0,
                .ArraySize = resourceDesc.DepthOrArraySize,
                .ResourceMinLODClamp = 0.0f,
            };
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            srvDesc.Texture2D =
            {
                .MostDetailedMip = 0,
                .MipLevels = resourceDesc.MipLevels,
                .PlaneSlice = 0,
                .ResourceMinLODClamp = 0.0f,
            };
        }
        else
        {
            srvDesc.Texture2DArray =
            {
                .MostDetailedMip = 0,
                .MipLevels = resourceDesc.MipLevels,
                .FirstArraySlice = 0,
                .ArraySize = resourceDesc.DepthOrArraySize,
                .PlaneSlice = 0,
                .ResourceMinLODClamp = 0.0f,
            };
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        srvDesc.Texture3D =
        {
            .MostDetailedMip = 0,
            .MipLevels = resourceDesc.MipLevels,
            .ResourceMinLODClamp = 0.0f,
        };
        break;
    }
    return srvDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC ResourceHandle::getDefaultUAVDesc()
{
    D3D12_RESOURCE_DESC resourceDesc = ResourceManager::it().getResourceDesc(*this);
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
    {
        .Format = resourceDesc.Format,
        .ViewDimension = resourceDimensionToUAVDimension(resourceDesc),
    };
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        assert(false);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            uavDesc.Texture1D =
            {
                .MipSlice = 0,
            };
        }
        else
        {
            uavDesc.Texture1DArray =
            {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = resourceDesc.DepthOrArraySize,
            };
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            uavDesc.Texture2D =
            {
                .MipSlice = 0,
                .PlaneSlice = 0,
            };
        }
        else
        {
            uavDesc.Texture2DArray =
            {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = resourceDesc.DepthOrArraySize,
                .PlaneSlice = 0,
            };
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        uavDesc.Texture3D =
        {
            .MipSlice = 0,
            .FirstWSlice = 0,
            .WSize = resourceDesc.DepthOrArraySize,
        };
        break;
    }
    return uavDesc;
}

D3D12_RENDER_TARGET_VIEW_DESC ResourceHandle::getDefaultRTVDesc()
{
    D3D12_RESOURCE_DESC resourceDesc = ResourceManager::it().getResourceDesc(*this);
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc =
    {
        .Format = resourceDesc.Format,
        .ViewDimension = resourceDimensionToRTVDimension(resourceDesc),
    };
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        assert(false);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            rtvDesc.Texture1D =
            {
                .MipSlice = 0,
            };
        }
        else
        {
            rtvDesc.Texture1DArray =
            {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = resourceDesc.DepthOrArraySize,
            };
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            rtvDesc.Texture2D =
            {
                .MipSlice = 0,
                .PlaneSlice = 0,
            };
        }
        else
        {
            rtvDesc.Texture2DArray =
            {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = resourceDesc.DepthOrArraySize,
                .PlaneSlice = 0,
            };
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        rtvDesc.Texture3D =
        {
            .MipSlice = 0,
            .FirstWSlice = 0,
            .WSize = resourceDesc.DepthOrArraySize,
        };
        break;
    }
    return rtvDesc;
}

D3D12_DEPTH_STENCIL_VIEW_DESC ResourceHandle::getDefaultDSVDesc()
{
    D3D12_RESOURCE_DESC resourceDesc = ResourceManager::it().getResourceDesc(*this);
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc =
    {
        .Format = resourceDesc.Format,
        .ViewDimension = resourceDimensionToDSVDimension(resourceDesc),
        .Flags = D3D12_DSV_FLAG_NONE,
    };
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    case D3D12_RESOURCE_DIMENSION_BUFFER:
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        assert(false);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            dsvDesc.Texture1D =
            {
                .MipSlice = 0,
            };
        }
        else
        {
            dsvDesc.Texture1DArray =
            {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = resourceDesc.DepthOrArraySize,
            };
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (resourceDesc.DepthOrArraySize == 1)
        {
            dsvDesc.Texture2D =
            {
                .MipSlice = 0,
            };
        }
        else
        {
            dsvDesc.Texture2DArray =
            {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = resourceDesc.DepthOrArraySize,
            };
        }
        break;
    }
    return dsvDesc;
}

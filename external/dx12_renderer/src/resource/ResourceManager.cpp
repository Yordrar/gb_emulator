#include "ResourceManager.h"

#include <Renderer.h>
#include <resource/Resource.h>
#include <resource/DescriptorHeap.h>
#include <resource/Descriptor.h>

ResourceManager::ResourceManager()
{
}

ResourceHandle ResourceManager::createResource( wchar_t const* name, D3D12_RESOURCE_DESC resourceDesc, D3D12_SUBRESOURCE_DATA const& subresourceData)
{
    if (m_freeResourceSlots.empty())
    {
        m_resources.push_back(ResourceContainerEntry{ std::move(Resource(name, resourceDesc, subresourceData)), 0 });
        return ResourceHandle{ static_cast<unsigned int>(m_resources.size()) - 1, 0 };
    }
    else
    {
        unsigned int slot = m_freeResourceSlots.front();
        m_freeResourceSlots.pop();
        m_resources[slot].m_resource = std::move(Resource(name, resourceDesc, subresourceData));
        return ResourceHandle{ slot, ++m_resources[slot].m_generation };
    }
}

ResourceHandle ResourceManager::createResource( wchar_t const* name, ComPtr<ID3D12Resource> resource)
{
    if (m_freeResourceSlots.empty())
    {
        m_resources.push_back(ResourceContainerEntry{ std::move(Resource(name, resource)), 0 });
        return ResourceHandle{ static_cast<unsigned int>(m_resources.size()) - 1, 0 };
    }
    else
    {
        unsigned int slot = m_freeResourceSlots.front();
        m_freeResourceSlots.pop();
        m_resources[slot].m_resource = std::move(Resource(name, resource));
        return ResourceHandle{ slot, m_resources[slot].m_generation };
    }
}

void ResourceManager::destroyResource(ResourceHandle handle)
{
    assert(isResourceHandleValid(handle));

    // Call the resource's destructor which releases the internal D3D resources and frees up video memory
    m_resources[handle.m_index].m_resource.~Resource();

    m_freeResourceSlots.push(handle.m_index);
    m_resources[handle.m_index].m_generation++;

    m_cbvs.erase(std::remove_if(m_cbvs.begin(), m_cbvs.end(), [&handle](Descriptor const& descriptor) { return descriptor.getResourceHandle() == handle; }), m_cbvs.end());
    m_srvs.erase(std::remove_if(m_srvs.begin(), m_srvs.end(), [&handle](Descriptor const& descriptor) { return descriptor.getResourceHandle() == handle; }), m_srvs.end());
    m_uavs.erase(std::remove_if(m_uavs.begin(), m_uavs.end(), [&handle](Descriptor const& descriptor) { return descriptor.getResourceHandle() == handle; }), m_uavs.end());
    m_rtvs.erase(std::remove_if(m_rtvs.begin(), m_rtvs.end(), [&handle](Descriptor const& descriptor) { return descriptor.getResourceHandle() == handle; }), m_rtvs.end());
    m_dsvs.erase(std::remove_if(m_dsvs.begin(), m_dsvs.end(), [&handle](Descriptor const& descriptor) { return descriptor.getResourceHandle() == handle; }), m_dsvs.end());
}

void ResourceManager::createSampler( wchar_t const* resourceName )
{
    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.MaxAnisotropy = 0;

    DescriptorHeap::getDescriptorHeapSampler().addSampler( &samplerDesc );
}

Descriptor const& ResourceManager::getConstantBufferView(ResourceHandle handle, D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc)
{
    assert(isResourceHandleValid(handle));
    for (Descriptor const& descriptor : m_cbvs)
    {
        if (memcmp(&descriptor.m_cbvDesc, &cbvDesc, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC)) == 0 &&
            descriptor.getResourceHandle() == handle)
        {
            return descriptor;
        }
    }

    cbvDesc.SizeInBytes = Resource::getSizeAligned256(cbvDesc.SizeInBytes);
    UINT descriptorIndex = DescriptorHeap::getDescriptorHeapCbvSrvUav().addCBV(&cbvDesc);
    Descriptor newCBV = Descriptor(Descriptor::Type::ConstantBufferView,
        handle,
        DescriptorHeap::getDescriptorHeapCbvSrvUav().getHeap()->GetCPUDescriptorHandleForHeapStart(),
        descriptorIndex,
        DescriptorHeap::getDescriptorHeapCbvSrvUav().getIncrementSize());
    newCBV.m_cbvDesc = cbvDesc;
    m_cbvs.push_back(newCBV);
    return m_cbvs[m_cbvs.size() - 1];
}

Descriptor const& ResourceManager::getShaderResourceView(ResourceHandle handle, D3D12_SHADER_RESOURCE_VIEW_DESC const& srvDesc)
{
    assert(isResourceHandleValid(handle));
    for (Descriptor const& descriptor : m_srvs)
    {
        if (memcmp(&descriptor.m_srvDesc, &srvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC)) == 0 &&
            descriptor.getResourceHandle() == handle)
        {
            return descriptor;
        }
    }

    UINT descriptorIndex = DescriptorHeap::getDescriptorHeapCbvSrvUav().addSRV(m_resources[handle.m_index].m_resource.getD3DResource(), &srvDesc);
    Descriptor newSRV = Descriptor(Descriptor::Type::ShaderResourceView,
        handle,
        DescriptorHeap::getDescriptorHeapCbvSrvUav().getHeap()->GetCPUDescriptorHandleForHeapStart(),
        descriptorIndex,
        DescriptorHeap::getDescriptorHeapCbvSrvUav().getIncrementSize());
    newSRV.m_srvDesc = srvDesc;
    m_srvs.push_back(newSRV);
    return m_srvs[m_srvs.size() - 1];
}

Descriptor const& ResourceManager::getUnorderedAccessView(ResourceHandle handle, D3D12_UNORDERED_ACCESS_VIEW_DESC const& uavDesc)
{
    assert(isResourceHandleValid(handle));
    for (Descriptor const& descriptor : m_uavs)
    {
        if (memcmp(&descriptor.m_uavDesc, &uavDesc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC)) == 0 &&
            descriptor.getResourceHandle() == handle)
        {
            return descriptor;
        }
    }

    UINT descriptorIndex = DescriptorHeap::getDescriptorHeapCbvSrvUav().addUAV(m_resources[handle.m_index].m_resource.getD3DResource(), &uavDesc);
    Descriptor newUAV = Descriptor(Descriptor::Type::UnorderedAccessView,
        handle,
        DescriptorHeap::getDescriptorHeapCbvSrvUav().getHeap()->GetCPUDescriptorHandleForHeapStart(),
        descriptorIndex,
        DescriptorHeap::getDescriptorHeapCbvSrvUav().getIncrementSize());
    newUAV.m_uavDesc = uavDesc;
    m_uavs.push_back(newUAV);
    return m_uavs[m_uavs.size() - 1];
}

Descriptor const& ResourceManager::getRenderTargetView(ResourceHandle handle, D3D12_RENDER_TARGET_VIEW_DESC const& rtvDesc)
{
    assert(isResourceHandleValid(handle));
    for (Descriptor const& descriptor : m_rtvs)
    {
        if (memcmp(&descriptor.m_rtvDesc, &rtvDesc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC)) == 0 &&
            descriptor.getResourceHandle() == handle)
        {
            return descriptor;
        }
    }

    UINT descriptorIndex = DescriptorHeap::getDescriptorHeapRtv().addRTV(m_resources[handle.m_index].m_resource.getD3DResource(), &rtvDesc);
    Descriptor newRTV = Descriptor(Descriptor::Type::RenderTargetView,
        handle,
        DescriptorHeap::getDescriptorHeapRtv().getHeap()->GetCPUDescriptorHandleForHeapStart(),
        descriptorIndex,
        DescriptorHeap::getDescriptorHeapRtv().getIncrementSize());
    newRTV.m_rtvDesc = rtvDesc;
    m_rtvs.push_back(newRTV);
    return m_rtvs[m_rtvs.size() - 1];
}

Descriptor const& ResourceManager::getDepthStencilView(ResourceHandle handle, D3D12_DEPTH_STENCIL_VIEW_DESC const& dsvDesc)
{
    assert(isResourceHandleValid(handle));
    for (Descriptor const& descriptor : m_dsvs)
    {
        if (memcmp(&descriptor.m_dsvDesc, &dsvDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC)) == 0 &&
            descriptor.getResourceHandle() == handle)
        {
            return descriptor;
        }
    }

    UINT descriptorIndex = DescriptorHeap::getDescriptorHeapDsv().addDSV(m_resources[handle.m_index].m_resource.getD3DResource(), &dsvDesc);
    Descriptor newDSV = Descriptor(Descriptor::Type::DepthStencilView,
        handle,
        DescriptorHeap::getDescriptorHeapDsv().getHeap()->GetCPUDescriptorHandleForHeapStart(),
        descriptorIndex,
        DescriptorHeap::getDescriptorHeapDsv().getIncrementSize());
    newDSV.m_dsvDesc = dsvDesc;
    m_dsvs.push_back(newDSV);
    return m_dsvs[m_dsvs.size() - 1];
}

ComPtr<ID3D12Resource> ResourceManager::getD3DResource(ResourceHandle handle) const
{
    assert(isResourceHandleValid(handle));
    return m_resources[handle.m_index].m_resource.getD3DResource();
}

D3D12_RESOURCE_STATES ResourceManager::getResourceState(ResourceHandle handle) const
{
    assert(isResourceHandleValid(handle));
    return m_resources[handle.m_index].m_resource.getResourceState();
}

D3D12_RESOURCE_BARRIER ResourceManager::getTransitionBarrier(ResourceHandle handle, D3D12_RESOURCE_STATES newState)
{
    assert(isResourceHandleValid(handle));
    return m_resources[handle.m_index].m_resource.getTransitionBarrier(newState);
}

D3D12_RESOURCE_DESC ResourceManager::getResourceDesc(ResourceHandle handle) const
{
    assert(isResourceHandleValid(handle));
    return m_resources[handle.m_index].m_resource.getResourceDesc();
}

void* ResourceManager::getResourceDataPtr(ResourceHandle handle)
{
    assert(isResourceHandleValid(handle));
    return (void*)m_resources[handle.m_index].m_resource.m_subresourceData.pData;
}

void ResourceManager::setResourceNeedsCopyToGPU(ResourceHandle handle)
{
    assert(isResourceHandleValid(handle));
    m_resources[handle.m_index].m_resource.setNeedsCopyToGPU(true);
}

void ResourceManager::copyResourcesToGPU( ComPtr<ID3D12GraphicsCommandList> commandList )
{
    std::vector<D3D12_RESOURCE_BARRIER> preCopyBarriers;
    std::vector<Resource*> resourcesToCopy;
    for ( ResourceContainer::value_type& resourceEntry : m_resources )
    {
        if (resourceEntry.m_resource.getNeedsCopyToGPU() )
        {
            resourcesToCopy.push_back( &resourceEntry.m_resource );
            if (resourceEntry.m_resource.getResourceState() != D3D12_RESOURCE_STATE_COPY_DEST )
            {
                D3D12_RESOURCE_BARRIER preCopyBarrier = resourceEntry.m_resource.getTransitionBarrier( D3D12_RESOURCE_STATE_COPY_DEST );
                preCopyBarriers.push_back( preCopyBarrier );
            }
        }
    }

    if (resourcesToCopy.size() > 0 )
    {
        PIXScopedEvent(commandList.Get(), PIX_COLOR_DEFAULT, "Copy resources to GPU");

        if (preCopyBarriers.size() > 0)
        {
            commandList->ResourceBarrier(static_cast<UINT>(preCopyBarriers.size()), preCopyBarriers.data());
        }

        for (Resource* resource : resourcesToCopy)
        {
            resource->copyDataToGPU(commandList);
        }
    }
}

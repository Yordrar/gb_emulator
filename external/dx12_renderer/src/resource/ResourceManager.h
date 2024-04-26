#pragma once

#include <Manager.h>
#include <resource/DescriptorHeap.h>
#include <resource/Resource.h>
#include <resource/ResourceHandle.h>

class ResourceManager : public Manager<ResourceManager>
{
    friend class Manager<ResourceManager>;
public:
    ~ResourceManager() = default;

    ResourceHandle createResource( wchar_t const* name, D3D12_RESOURCE_DESC resourceDesc, D3D12_SUBRESOURCE_DATA const& subresourceData = D3D12_SUBRESOURCE_DATA{ nullptr, 0, 0 });
    ResourceHandle createResource( wchar_t const* name, ComPtr<ID3D12Resource> resource);
    void destroyResource(ResourceHandle handle);

    void createSampler( wchar_t const* resourceName );

    Descriptor const& getConstantBufferView(ResourceHandle handle, D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc);
    Descriptor const& getShaderResourceView(ResourceHandle handle, D3D12_SHADER_RESOURCE_VIEW_DESC const& srvDesc);
    Descriptor const& getUnorderedAccessView(ResourceHandle handle, D3D12_UNORDERED_ACCESS_VIEW_DESC const& uavDesc);
    Descriptor const& getRenderTargetView(ResourceHandle handle, D3D12_RENDER_TARGET_VIEW_DESC const& rtvDesc);
    Descriptor const& getDepthStencilView(ResourceHandle handle, D3D12_DEPTH_STENCIL_VIEW_DESC const& dsvDesc);

    ComPtr<ID3D12Resource> getD3DResource(ResourceHandle handle) const;
    D3D12_RESOURCE_STATES getResourceState(ResourceHandle handle) const;
    D3D12_RESOURCE_BARRIER getTransitionBarrier(ResourceHandle handle, D3D12_RESOURCE_STATES newState);
    D3D12_RESOURCE_DESC getResourceDesc(ResourceHandle handle) const;
    void setResourceNeedsCopyToGPU(ResourceHandle handle);

    void copyResourcesToGPU( ComPtr<ID3D12GraphicsCommandList> commandList );

    bool isResourceHandleValid(ResourceHandle handle) const { return handle.isValid(); }
    bool doesResourceHandlePointToValidResource(ResourceHandle handle) const { return m_resources[handle.m_index].m_generation == handle.m_generation; }

private:
    ResourceManager();

    struct ResourceContainerEntry
    {
        Resource m_resource;
        unsigned int m_generation;
    };
    using ResourceContainer = std::vector<ResourceContainerEntry>;
    ResourceContainer m_resources;
    std::queue<unsigned int> m_freeResourceSlots;

    using ResourceViewContainer = std::vector<Descriptor>;
    ResourceViewContainer m_cbvs;
    ResourceViewContainer m_srvs;
    ResourceViewContainer m_uavs;
    ResourceViewContainer m_rtvs;
    ResourceViewContainer m_dsvs;
};

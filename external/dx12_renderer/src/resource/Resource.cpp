#include "Resource.h"

#include <Renderer.h>
#include <resource/Descriptor.h>
#include <resource/DescriptorHeap.h>

Resource::Resource( wchar_t const* name, D3D12_RESOURCE_DESC& resourceDesc, D3D12_SUBRESOURCE_DATA const& subresourceData )
    : m_name( name )
    , m_resource( nullptr )
    , m_intermediateUploadBuffer( nullptr )
    , m_subresourceData( subresourceData )
    , m_resourceState( D3D12_RESOURCE_STATE_COMMON )
    , m_needsCopyToGPU( subresourceData.pData != nullptr )
{
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        resourceDesc.Width = Resource::getSizeAligned256(static_cast<UINT>(resourceDesc.Width));
    }

    FLOAT clearColor[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
    CD3DX12_CLEAR_VALUE clearValue;
    CD3DX12_CLEAR_VALUE* clearValuePtr = nullptr;

    if ( resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET )
    {
        clearValue = CD3DX12_CLEAR_VALUE( resourceDesc.Format, clearColor );
        clearValuePtr = &clearValue;
    }
    else if ( resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL )
    {
        clearValue = CD3DX12_CLEAR_VALUE( resourceDesc.Format, 1.0f, 0 );
        clearValuePtr = &clearValue;
    }

    CD3DX12_HEAP_PROPERTIES heapProperties( D3D12_HEAP_TYPE_DEFAULT );
    Renderer::device()->CreateCommittedResource(&heapProperties,
                                                 D3D12_HEAP_FLAG_NONE,
                                                 &resourceDesc,
                                                 D3D12_RESOURCE_STATE_COMMON,
                                                 clearValuePtr,
                                                 IID_PPV_ARGS( m_resource.GetAddressOf() ) );

    CD3DX12_RESOURCE_DESC uploadBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer( GetRequiredIntermediateSize( m_resource.Get(), 0, 1 ) );
    CD3DX12_HEAP_PROPERTIES uploadHeapProperties( D3D12_HEAP_TYPE_UPLOAD );
    Renderer::device()->CreateCommittedResource( &uploadHeapProperties,
                                                 D3D12_HEAP_FLAG_NONE,
                                                 &uploadBufferResourceDesc,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ,
                                                 nullptr,
                                                 IID_PPV_ARGS( m_intermediateUploadBuffer.GetAddressOf() ) );
    m_resource->SetName(name);
}

Resource::Resource( wchar_t const* name, ComPtr<ID3D12Resource> resource )
    : m_name( name )
    , m_resource( resource )
    , m_intermediateUploadBuffer( nullptr )
    , m_subresourceData( D3D12_SUBRESOURCE_DATA{ nullptr, 0, 0 } )
    , m_resourceState( D3D12_RESOURCE_STATE_COMMON )
    , m_needsCopyToGPU( false )
{

    CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
    CD3DX12_RESOURCE_DESC uploadBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer( GetRequiredIntermediateSize( m_resource.Get(), 0, 1 ) );
    Renderer::device()->CreateCommittedResource( &uploadHeapProperties,
                                                 D3D12_HEAP_FLAG_NONE,
                                                 &uploadBufferResourceDesc,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ,
                                                 nullptr,
                                                 IID_PPV_ARGS( m_intermediateUploadBuffer.GetAddressOf() ) );
    m_resource->SetName(name);
}

Resource::~Resource()
{
    m_resource.Reset();
    m_intermediateUploadBuffer.Reset();
}

D3D12_RESOURCE_BARRIER Resource::getTransitionBarrier( D3D12_RESOURCE_STATES newState )
{
    D3D12_RESOURCE_BARRIER barrier =
    {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition =
        {
            .pResource = m_resource.Get(),
            .Subresource = 0,
            .StateBefore = m_resourceState,
            .StateAfter = newState,
        },
    };

    m_resourceState = newState;

    return barrier;
}

void Resource::copyDataToGPU( ComPtr<ID3D12GraphicsCommandList> commandList )
{
    assert( m_resourceState == D3D12_RESOURCE_STATE_COPY_DEST );
    assert( m_subresourceData.pData );
    if (m_resource && m_intermediateUploadBuffer)
    {
        UpdateSubresources<1>(commandList.Get(), m_resource.Get(), m_intermediateUploadBuffer.Get(), 0, 0, 1, &m_subresourceData);
        m_needsCopyToGPU = false;
    }
}
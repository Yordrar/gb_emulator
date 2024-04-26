#include "DescriptorHeap.h"

#include <Renderer.h>

std::unique_ptr<DescriptorHeap> DescriptorHeap::s_descriptorHeapCbvSrvUav = nullptr;
std::unique_ptr<DescriptorHeap> DescriptorHeap::s_descriptorHeapSampler = nullptr;
std::unique_ptr<DescriptorHeap> DescriptorHeap::s_descriptorHeapRtv = nullptr;
std::unique_ptr<DescriptorHeap> DescriptorHeap::s_descriptorHeapDsv = nullptr;

DescriptorHeap::DescriptorHeap( D3D12_DESCRIPTOR_HEAP_DESC heapDesc )
    : m_type(heapDesc.Type)
    , m_nextFreeSlot(0)
    , m_numSlots(heapDesc.NumDescriptors)
{
    Renderer::device()->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( m_heap.GetAddressOf() ) );
}

UINT DescriptorHeap::addSRV( ComPtr<ID3D12Resource> resource, D3D12_SHADER_RESOURCE_VIEW_DESC const* srv )
{
    assert( m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    UINT slot = getNextSlot();

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle( m_heap->GetCPUDescriptorHandleForHeapStart(), slot, getIncrementSize() );
    Renderer::device()->CreateShaderResourceView( resource.Get(), srv, handle );

    return slot;
}

UINT DescriptorHeap::getIncrementSize() const
{
    return Renderer::device()->GetDescriptorHandleIncrementSize(m_type);
}

UINT DescriptorHeap::addCBV( D3D12_CONSTANT_BUFFER_VIEW_DESC const* cbv )
{
    assert( m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    UINT slot = getNextSlot();

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle( m_heap->GetCPUDescriptorHandleForHeapStart(), slot, getIncrementSize() );
    Renderer::device()->CreateConstantBufferView( cbv, handle );

    return slot;
}

UINT DescriptorHeap::addUAV( ComPtr<ID3D12Resource> resource, D3D12_UNORDERED_ACCESS_VIEW_DESC const* uav )
{
    assert( m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    UINT slot = getNextSlot();

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle( m_heap->GetCPUDescriptorHandleForHeapStart(), slot, getIncrementSize() );
    Renderer::device()->CreateUnorderedAccessView( resource.Get(), nullptr, uav, handle );

    return slot;
}

UINT DescriptorHeap::addSampler( D3D12_SAMPLER_DESC* samplerDesc )
{
    assert( m_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER );

    UINT slot = getNextSlot();

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle( m_heap->GetCPUDescriptorHandleForHeapStart(), slot, getIncrementSize() );
    Renderer::device()->CreateSampler( samplerDesc, handle );

    return slot;
}

UINT DescriptorHeap::addRTV( ComPtr<ID3D12Resource> resource, D3D12_RENDER_TARGET_VIEW_DESC const* rtv )
{
    assert( m_type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

    UINT slot = getNextSlot();

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle( m_heap->GetCPUDescriptorHandleForHeapStart(), slot, getIncrementSize() );
    Renderer::device()->CreateRenderTargetView( resource.Get(), rtv, handle );

    return slot;
}

UINT DescriptorHeap::addDSV( ComPtr<ID3D12Resource> resource, D3D12_DEPTH_STENCIL_VIEW_DESC const* dsv )
{
    assert( m_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV );

    UINT slot = getNextSlot();

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle( m_heap->GetCPUDescriptorHandleForHeapStart(), slot, getIncrementSize() );
    Renderer::device()->CreateDepthStencilView( resource.Get(), dsv, handle );

    return slot;
}

void DescriptorHeap::removeDescriptor( Descriptor const& descriptor )
{
    m_freeSlots.push( descriptor.getDescriptorIndex() );
}

DescriptorHeap& DescriptorHeap::getDescriptorHeapCbvSrvUav()
{
    if ( !s_descriptorHeapCbvSrvUav )
    {
        s_descriptorHeapCbvSrvUav = std::make_unique<DescriptorHeap>( D3D12_DESCRIPTOR_HEAP_DESC
                                                                      {
                                                                          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                          NUM_DESCRIPTORS_IN_DESCRIPTOR_HEAPS,
                                                                          D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                                                      } );
    }

    return *s_descriptorHeapCbvSrvUav;
}

DescriptorHeap& DescriptorHeap::getDescriptorHeapSampler()
{
    if ( !s_descriptorHeapSampler )
    {
        s_descriptorHeapSampler = std::make_unique<DescriptorHeap>( D3D12_DESCRIPTOR_HEAP_DESC
                                                                    {
                                                                        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                                                                        NUM_DESCRIPTORS_IN_DESCRIPTOR_HEAPS,
                                                                        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                                                    } );
    }

    return *s_descriptorHeapSampler;
}

DescriptorHeap& DescriptorHeap::getDescriptorHeapRtv()
{
    if ( !s_descriptorHeapRtv )
    {
        s_descriptorHeapRtv = std::make_unique<DescriptorHeap>( D3D12_DESCRIPTOR_HEAP_DESC
                                                                {
                                                                    D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                                                                    NUM_DESCRIPTORS_IN_DESCRIPTOR_HEAPS,
                                                                    D3D12_DESCRIPTOR_HEAP_FLAG_NONE
                                                                } );
    }

    return *s_descriptorHeapRtv;
}

DescriptorHeap& DescriptorHeap::getDescriptorHeapDsv()
{
    if ( !s_descriptorHeapDsv )
    {
        s_descriptorHeapDsv = std::make_unique<DescriptorHeap>( D3D12_DESCRIPTOR_HEAP_DESC
                                                                {
                                                                    D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
                                                                    NUM_DESCRIPTORS_IN_DESCRIPTOR_HEAPS,
                                                                    D3D12_DESCRIPTOR_HEAP_FLAG_NONE
                                                                } );
    }

    return *s_descriptorHeapDsv;
}

UINT DescriptorHeap::getNextSlot()
{
    UINT slot = 0;

    if ( m_freeSlots.empty() )
    {
        slot = m_nextFreeSlot++;
    }
    else
    {
        slot = m_freeSlots.front();
        m_freeSlots.pop();
    }

    return slot;
}

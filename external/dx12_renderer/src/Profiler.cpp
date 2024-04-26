#include "Profiler.h"

#include <Renderer.h>
#include <RendererConstants.h>

Profiler::Profiler( Renderer const& renderer )
    : m_renderer( renderer )
    , m_queryHeap( nullptr )
    , m_resolvedQueriesResource( nullptr )
    , m_numAllocatedQueryIndices( 0 )
{
    D3D12_QUERY_HEAP_DESC queryHeapDesc =
    {
        .Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
        .Count = 2 * sc_numQueriesPerFrame * RendererConstants::sc_numBackBuffers,
        .NodeMask = 0,
    };
    Renderer::device()->CreateQueryHeap( &queryHeapDesc, IID_PPV_ARGS( m_queryHeap.GetAddressOf() ) );

    CD3DX12_HEAP_PROPERTIES resolvedQueriesResourceHeapProperties = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_READBACK, 0, 0 );
    CD3DX12_RESOURCE_DESC resolvedQueriesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer( queryHeapDesc.Count * sizeof( uint64_t ) );
    Renderer::device()->CreateCommittedResource( &resolvedQueriesResourceHeapProperties,
                                                 D3D12_HEAP_FLAG_NONE,
                                                 &resolvedQueriesResourceDesc,
                                                 D3D12_RESOURCE_STATE_COPY_DEST,
                                                 nullptr,
                                                 IID_PPV_ARGS( m_resolvedQueriesResource.GetAddressOf() ) );
}

uint64_t Profiler::getInHeapQueryIndexForCurrentFrameFromAllocatedIndex( uint64_t allocatedIndex )
{
    return (2 * allocatedIndex) + (2 * sc_numQueriesPerFrame * m_renderer.getCurrentBackbufferIndex());
}

uint64_t Profiler::getInHeapQueryIndexForPreviousFrameFromAllocatedIndex( uint64_t allocatedIndex )
{
    return (2 * allocatedIndex) + (2 * sc_numQueriesPerFrame * m_renderer.getPreviousBackbufferIndex());
}

uint64_t Profiler::allocateQueryIndex()
{
    return m_numAllocatedQueryIndices++;
}

void Profiler::startQuery( ComPtr<ID3D12GraphicsCommandList> commandList, uint64_t index )
{
    commandList->EndQuery( m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, (UINT)getInHeapQueryIndexForCurrentFrameFromAllocatedIndex(index) );
}

void Profiler::endQuery( ComPtr<ID3D12GraphicsCommandList> commandList, uint64_t index )
{
    commandList->EndQuery( m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, (UINT)getInHeapQueryIndexForCurrentFrameFromAllocatedIndex(index) + 1 );

    uint64_t const dstOffset = getInHeapQueryIndexForCurrentFrameFromAllocatedIndex( index ) * sizeof( uint64_t );
    commandList->ResolveQueryData( m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, (UINT)getInHeapQueryIndexForCurrentFrameFromAllocatedIndex(index), 2, m_resolvedQueriesResource.Get(), dstOffset );
}

double Profiler::getResolvedQuery( uint64_t index )
{
    void* data = nullptr;
    D3D12_RANGE readRange =
    {
        .Begin = getInHeapQueryIndexForPreviousFrameFromAllocatedIndex(index) * sizeof(uint64_t),
        .End = readRange.Begin + 2 * sizeof(uint64_t),
    };
    m_resolvedQueriesResource->Map( 0, &readRange, &data );
    uint64_t* resolvedQueries = reinterpret_cast<uint64_t*>( data );
    double gpuFrequency = static_cast<double>(m_renderer.getTimestampFrequency() );

    uint64_t start = resolvedQueries[ getInHeapQueryIndexForPreviousFrameFromAllocatedIndex( index ) ];
    uint64_t end = resolvedQueries[ getInHeapQueryIndexForPreviousFrameFromAllocatedIndex( index ) + 1 ];

    D3D12_RANGE writtenRange =
    {
        .Begin = 0,
        .End = 0,
    };
    m_resolvedQueriesResource->Unmap( 0, &writtenRange );

    return ( ( end - start ) / gpuFrequency ) * 1000.0;
}

#include "Fence.h"

#include <Renderer.h>

Fence::Fence( wchar_t const* name )
{
    Renderer::device()->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_fence ) );

    m_fenceEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    m_fence->SetName( name );
}

Fence::~Fence()
{

}

HRESULT Fence::CPUSignal( uint64_t fenceValue )
{
    return m_fence->Signal( fenceValue );
}

DWORD Fence::CPUWait( uint64_t fenceValue, uint64_t duration )
{
    if ( m_fence->GetCompletedValue() < fenceValue )
    {
        m_fence->SetEventOnCompletion( fenceValue, m_fenceEvent );
        DWORD result = WaitForSingleObject( m_fenceEvent, static_cast<DWORD>( duration ) );
        PIXNotifyWakeFromFenceSignal( m_fenceEvent );
        return result;
    }
    return 0;
}

HRESULT Fence::GPUSignal( ComPtr<ID3D12CommandQueue> commandQueue, uint64_t fenceValue )
{
    return commandQueue->Signal( m_fence.Get(), fenceValue );
}

HRESULT Fence::GPUWait( ComPtr<ID3D12CommandQueue> commandQueue, uint64_t fenceValue )
{
    return commandQueue->Wait( m_fence.Get(), fenceValue );
}
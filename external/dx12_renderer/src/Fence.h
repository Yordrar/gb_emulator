#pragma once

#include <Renderer.h>

class Fence
{
public:
    Fence( wchar_t const* name );
    ~Fence();

    HRESULT CPUSignal( uint64_t fenceValue = Renderer::getGlobalFrameCounter() );
    DWORD CPUWait( uint64_t fenceValue = Renderer::getGlobalFrameCounter(), uint64_t duration = 0xFFFFFFFFFFFFFFFF);
    HRESULT GPUSignal( ComPtr<ID3D12CommandQueue> commandQueue, uint64_t fenceValue = Renderer::getGlobalFrameCounter() );
    HRESULT GPUWait( ComPtr<ID3D12CommandQueue> commandQueue, uint64_t fenceValue = Renderer::getGlobalFrameCounter() );

    uint64_t getCompletedValue() const { return m_fence->GetCompletedValue(); }

private:
    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fenceEvent;
};


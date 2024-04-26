#include <BarrierRecorder.h>

#include <resource/ResourceManager.h>

void BarrierRecorder::recordBarrierTransition(ResourceHandle handle, D3D12_RESOURCE_STATES newState)
{
    if (ResourceManager::it().getResourceState(handle) != newState)
    {
        D3D12_RESOURCE_BARRIER barrier = ResourceManager::it().getTransitionBarrier(handle, newState);
        m_recordedBarriers.push_back(barrier);
    }
}

void BarrierRecorder::recordBarrierUAV(ResourceHandle handle)
{
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(ResourceManager::it().getD3DResource(handle).Get());
    m_recordedBarriers.push_back(barrier);
}

void BarrierRecorder::submitBarriers(ComPtr<ID3D12GraphicsCommandList> commandList)
{
    if (m_recordedBarriers.size() > 0)
    {
        commandList->ResourceBarrier(static_cast<UINT>(m_recordedBarriers.size()), m_recordedBarriers.data());
        m_recordedBarriers.clear();
    }
}

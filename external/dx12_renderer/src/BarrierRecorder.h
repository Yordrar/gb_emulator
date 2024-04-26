#pragma once

#include <resource/ResourceHandle.h>

class BarrierRecorder
{
public:
    BarrierRecorder() = default;
    ~BarrierRecorder() = default;

    void recordBarrierTransition(ResourceHandle handle, D3D12_RESOURCE_STATES newState);
    void recordBarrierUAV(ResourceHandle handle);

    void submitBarriers(ComPtr<ID3D12GraphicsCommandList> commandList);
private:
    std::vector<D3D12_RESOURCE_BARRIER> m_recordedBarriers;
};
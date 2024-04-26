#pragma once

#include <queue>

#include <resource/Descriptor.h>

class DescriptorHeap
{
public:
    static constexpr size_t NUM_DESCRIPTORS_IN_DESCRIPTOR_HEAPS = 1024;

    DescriptorHeap( D3D12_DESCRIPTOR_HEAP_DESC heapDesc );

    ComPtr<ID3D12DescriptorHeap> getHeap() const { return m_heap; }
    UINT getIncrementSize() const;

    UINT addCBV( D3D12_CONSTANT_BUFFER_VIEW_DESC const* cbv );
    UINT addSRV( ComPtr<ID3D12Resource> resource, D3D12_SHADER_RESOURCE_VIEW_DESC const* srv );
    UINT addUAV( ComPtr<ID3D12Resource> resource, D3D12_UNORDERED_ACCESS_VIEW_DESC const* uav );
    UINT addSampler( D3D12_SAMPLER_DESC* samplerDesc );
    UINT addRTV( ComPtr<ID3D12Resource> resource, D3D12_RENDER_TARGET_VIEW_DESC const* rtv );
    UINT addDSV( ComPtr<ID3D12Resource> resource, D3D12_DEPTH_STENCIL_VIEW_DESC const* dsv );

    void removeDescriptor( Descriptor const& descriptor );

    static DescriptorHeap& getDescriptorHeapCbvSrvUav();
    static DescriptorHeap& getDescriptorHeapSampler();
    static DescriptorHeap& getDescriptorHeapRtv();
    static DescriptorHeap& getDescriptorHeapDsv();

private:
    UINT getNextSlot();

    static std::unique_ptr<DescriptorHeap> s_descriptorHeapCbvSrvUav;
    static std::unique_ptr<DescriptorHeap> s_descriptorHeapSampler;
    static std::unique_ptr<DescriptorHeap> s_descriptorHeapRtv;
    static std::unique_ptr<DescriptorHeap> s_descriptorHeapDsv;

    ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    std::queue<UINT> m_freeSlots;
    UINT m_nextFreeSlot;
    UINT m_numSlots;
};

#pragma once

class Renderer;

class Profiler
{
public:
	Profiler( Renderer const& renderer );
	~Profiler() = default;

	static constexpr uint64_t sc_numQueriesPerFrame = 1024;

	uint64_t allocateQueryIndex();
	void startQuery( ComPtr<ID3D12GraphicsCommandList> commandList, uint64_t index );
	void endQuery( ComPtr<ID3D12GraphicsCommandList> commandList, uint64_t index );
	double getResolvedQuery( uint64_t index );

private:
	uint64_t getInHeapQueryIndexForCurrentFrameFromAllocatedIndex( uint64_t allocatedIndex );
	uint64_t getInHeapQueryIndexForPreviousFrameFromAllocatedIndex( uint64_t allocatedIndex );

	Renderer const& m_renderer;

	ComPtr<ID3D12QueryHeap> m_queryHeap;
	ComPtr<ID3D12Resource> m_resolvedQueriesResource;
	uint64_t m_numAllocatedQueryIndices;
};

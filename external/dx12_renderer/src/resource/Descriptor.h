#pragma once

#include <d3d12.h>

#include <resource/ResourceHandle.h>

class Descriptor
{
	friend class ResourceManager;
public:
	enum class Type
	{
		InvalidView,
		ShaderResourceView,
		ConstantBufferView,
		UnorderedAccessView,
		RenderTargetView,
		DepthStencilView,
	};

	Descriptor();
	Descriptor( Type type,
				ResourceHandle handle,
				D3D12_CPU_DESCRIPTOR_HANDLE const& cpuDescriptorHandleForHeapStart,
				UINT offsetInDescriptors,
				UINT descriptorIncrementSize );
	~Descriptor();

	operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_descriptorHandle; }
	operator UINT() const { return m_descriptorIndex; }

	bool isValid() const { return m_type != Type::InvalidView && m_resource.isValid(); }

	Type getType() const { return m_type; }
	ResourceHandle getResourceHandle() const { return m_resource; }
	D3D12_CPU_DESCRIPTOR_HANDLE getDescriptorHandle() const { return m_descriptorHandle; }
	UINT getDescriptorIndex() const { return m_descriptorIndex; }

	D3D12_CONSTANT_BUFFER_VIEW_DESC getCBVDesc() const { return m_cbvDesc; }
	D3D12_SHADER_RESOURCE_VIEW_DESC getSRVDesc() const { return m_srvDesc; }
	D3D12_UNORDERED_ACCESS_VIEW_DESC getUAVDesc() const { return m_uavDesc; }
	D3D12_RENDER_TARGET_VIEW_DESC getRTVDesc() const { return m_rtvDesc; }
	D3D12_DEPTH_STENCIL_VIEW_DESC getDSVDesc() const { return m_dsvDesc; }

private:
	Type m_type;
	ResourceHandle m_resource;
	D3D12_CPU_DESCRIPTOR_HANDLE m_descriptorHandle;
	UINT m_descriptorIndex;

	union
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbvDesc;
		D3D12_SHADER_RESOURCE_VIEW_DESC m_srvDesc;
		D3D12_UNORDERED_ACCESS_VIEW_DESC m_uavDesc;
		D3D12_RENDER_TARGET_VIEW_DESC m_rtvDesc;
		D3D12_DEPTH_STENCIL_VIEW_DESC m_dsvDesc;
	};
};


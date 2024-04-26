#pragma once

#include <resource/ResourceHandle.h>

class IndexBuffer
{
public:
	IndexBuffer( wchar_t const* name, UINT* indices, UINT count );
	virtual ~IndexBuffer();

	void bind( ComPtr<ID3D12GraphicsCommandList> commandList );

	UINT getIndexCount() const { return m_indexCount; }
	ResourceHandle getResource() const { return m_resource; }

private:
	UINT* m_indices;
	UINT m_indexCount;
	ResourceHandle m_resource;
};

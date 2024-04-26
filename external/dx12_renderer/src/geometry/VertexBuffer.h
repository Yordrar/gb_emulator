#pragma once

#include <resource/ResourceHandle.h>

class VertexBuffer
{
public:
	VertexBuffer( wchar_t const* name, void* vertices, size_t vertexSize, size_t vertexCount );
	virtual ~VertexBuffer();

	void bind( ComPtr<ID3D12GraphicsCommandList> commandList );

	size_t getVertexCount() const { return m_vertexCount; }
	ResourceHandle getResource() const { return m_resource; }

private:
	void* m_vertices;
	size_t m_vertexSize;
	size_t m_vertexCount;
	ResourceHandle m_resource;
};
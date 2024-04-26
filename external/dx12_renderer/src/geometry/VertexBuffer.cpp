#include "VertexBuffer.h"

#include <Renderer.h>
#include <resource/ResourceManager.h>

VertexBuffer::VertexBuffer( wchar_t const* name, void* vertices, size_t vertexSize, size_t vertexCount )
	: m_vertices( new uint8_t[ vertexCount * vertexSize ] )
	, m_vertexSize( vertexSize )
	, m_vertexCount( vertexCount )
{
	memcpy( m_vertices, vertices, m_vertexCount * m_vertexSize );

	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer( m_vertexCount * m_vertexSize );
	D3D12_SUBRESOURCE_DATA subresData =
	{
		.pData = m_vertices,
		.RowPitch = static_cast<INT64>( m_vertexCount ) * static_cast<INT64>( m_vertexSize ),
		.SlicePitch = 0,
	};
	m_resource = ResourceManager::it().createResource( name, resourceDesc, subresData );
}

VertexBuffer::~VertexBuffer()
{
	delete[] m_vertices;
}

void VertexBuffer::bind( ComPtr<ID3D12GraphicsCommandList> commandList )
{
	D3D12_VERTEX_BUFFER_VIEW view =
	{
		.BufferLocation = ResourceManager::it().getD3DResource(m_resource)->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>( m_vertexSize * m_vertexCount ),
		.StrideInBytes = static_cast<UINT>( m_vertexSize ),
	};
	commandList->IASetVertexBuffers( 0, 1, &view );
}
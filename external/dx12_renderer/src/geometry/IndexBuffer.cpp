#include "IndexBuffer.h"

#include <Renderer.h>
#include <resource/ResourceManager.h>

IndexBuffer::IndexBuffer( wchar_t const* name, UINT* indices, UINT count )
	: m_indices( new UINT[ count ] )
	, m_indexCount( count )
{
	memcpy( m_indices, indices, m_indexCount * sizeof( UINT ) );

	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer( m_indexCount * sizeof( UINT ) );
	D3D12_SUBRESOURCE_DATA subresData =
	{
		.pData = m_indices,
		.RowPitch = m_indexCount * sizeof( UINT ),
		.SlicePitch = 0,
	};
	m_resource = ResourceManager::it().createResource( name, resourceDesc, subresData );
}

IndexBuffer::~IndexBuffer()
{
	delete[] m_indices;
}

void IndexBuffer::bind( ComPtr<ID3D12GraphicsCommandList> commandList )
{
	D3D12_INDEX_BUFFER_VIEW view =
	{
		.BufferLocation = ResourceManager::it().getD3DResource(m_resource)->GetGPUVirtualAddress(),
		.SizeInBytes = sizeof( UINT ) * m_indexCount,
		.Format = DXGI_FORMAT_R32_UINT,
	};
	commandList->IASetIndexBuffer(&view);
}

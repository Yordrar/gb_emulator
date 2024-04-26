#pragma once

#include <limits>
#include <memory>

#include <DirectXMath.h>

#include <geometry/Material.h>
#include <geometry/VertexBuffer.h>
#include <geometry/IndexBuffer.h>

class Mesh
{
public:
    struct AABB
    {
        DirectX::XMVECTOR m_minBounds = DirectX::XMVectorSet( std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1.0f );
        DirectX::XMVECTOR m_maxBounds = DirectX::XMVectorSet( std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), 1.0f );
    };

    Mesh( wchar_t const* name, wchar_t const* materialName, D3D_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    ~Mesh() = default;

    void setVertexBuffer( void* vertexData, UINT vertexSize, UINT vertexCount );
    bool hasVertexBuffer() const { return m_vertexBuffer != nullptr; }
    void setIndexBuffer( UINT* indexData, UINT indexCount );
    void setAABB( AABB const& aabb ) { m_aabb = aabb; }
    bool isAABBValid() const;

    std::wstring const& getMaterialName() const { return m_materialName; }
    AABB const& getAABB() const { return m_aabb; }

    void record( ComPtr<ID3D12GraphicsCommandList> commandList );

private:
    std::wstring m_name;
    std::wstring m_materialName;
    std::unique_ptr<VertexBuffer> m_vertexBuffer;
    std::unique_ptr<IndexBuffer> m_indexBuffer;
    AABB m_aabb;
    D3D_PRIMITIVE_TOPOLOGY m_primitiveTopology;
};
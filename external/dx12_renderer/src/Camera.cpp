#include "Camera.h"

#include <resource/ResourceManager.h>

Camera::Camera( wchar_t const* name, DirectX::XMVECTOR position, float fov, float aspect_ratio, float nearZ, float farZ )
	: m_name( name )
	, m_localRightVector( DirectX::XMVectorSet( 1, 0, 0, 0 ) )
	, m_localUpVector( DirectX::XMVectorSet( 0, 1, 0, 0 ) )
	, m_localForwardVector( DirectX::XMVectorSet( 0, 0, 1, 0 ) )
	, m_aspectRatio( aspect_ratio )
	, m_fov( fov )
	, m_nearZ( nearZ )
	, m_farZ( farZ )
{
	m_cameraData.m_position = position;
	m_cameraData.m_viewProjMatrix = DirectX::XMMatrixTranspose( DirectX::XMMatrixLookToRH( m_cameraData.m_position, m_localForwardVector, m_localUpVector ) * DirectX::XMMatrixPerspectiveFovRH( fov, aspect_ratio, m_nearZ, m_farZ ) );
	m_cameraData.m_inverseViewProjMatrix = DirectX::XMMatrixInverse( nullptr, m_cameraData.m_viewProjMatrix );

	m_cameraBuffer = ResourceManager::it().createResource( std::wstring(m_name + L"_buffer").c_str(), CD3DX12_RESOURCE_DESC::Buffer( sizeof( m_cameraData ) ), D3D12_SUBRESOURCE_DATA{ &m_cameraData, sizeof( m_cameraData ), 0 } );
	D3D12_RESOURCE_DESC cameraBufferResourceDesc = ResourceManager::it().getResourceDesc(m_cameraBuffer);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
	{
		.Format = DXGI_FORMAT_R32_TYPELESS,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	};
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	srvDesc.Buffer.NumElements = sizeof(m_cameraData)/sizeof(UINT);
	srvDesc.Buffer.StructureByteStride = 0;
	m_cameraBufferDescriptor = ResourceManager::it().getShaderResourceView(m_cameraBuffer, srvDesc);
}

void Camera::move( float delta_x, float delta_y, float delta_z )
{
	m_cameraData.m_position.m128_f32[0] += delta_x;
	m_cameraData.m_position.m128_f32[1] += delta_y;
	m_cameraData.m_position.m128_f32[2] += delta_z;

	m_cameraData.m_viewProjMatrix = DirectX::XMMatrixTranspose( DirectX::XMMatrixLookToRH( m_cameraData.m_position, m_localForwardVector, m_localUpVector ) * DirectX::XMMatrixPerspectiveFovRH( m_fov, m_aspectRatio, m_nearZ, m_farZ ));
	m_cameraData.m_inverseViewProjMatrix = DirectX::XMMatrixInverse( nullptr, m_cameraData.m_viewProjMatrix );

	ResourceManager::it().setResourceNeedsCopyToGPU(m_cameraBuffer);
}

void Camera::rotate( float delta_angles_x, float delta_angles_y )
{
	// Create rotation quaternion for x axis
	float angle_x_rad = DirectX::XMConvertToRadians( delta_angles_x / 2.0f);
	DirectX::XMFLOAT3 quaternion_x_imaginary(sinf(angle_x_rad) * m_localRightVector.m128_f32[0], sinf(angle_x_rad) * m_localRightVector.m128_f32[1], sinf(angle_x_rad) * m_localRightVector.m128_f32[2]);
	float quaternion_x_real = cosf(angle_x_rad);
	DirectX::XMVECTOR quaternion_x = DirectX::XMVectorSet(quaternion_x_imaginary.x, quaternion_x_imaginary.y, quaternion_x_imaginary.z, quaternion_x_real);

	// Create rotation quaternion for y axis
	float angle_y_rad = DirectX::XMConvertToRadians( delta_angles_y / 2.0f);
	DirectX::XMFLOAT3 quaternion_y_imaginary(0, sinf(angle_y_rad), 0);
	float quaternion_y_real = cosf(angle_y_rad);
	DirectX::XMVECTOR quaternion_y = DirectX::XMVectorSet(quaternion_y_imaginary.x, quaternion_y_imaginary.y, quaternion_y_imaginary.z, quaternion_y_real);

	// Combine quaternions
	DirectX::XMVECTOR quaternion = DirectX::XMQuaternionNormalize(DirectX::XMQuaternionMultiply(quaternion_x, quaternion_y));

	//Apply result to basis vectors and position
	{
		DirectX::XMVECTOR intermediate_result = DirectX::XMQuaternionMultiply( quaternion, m_cameraData.m_position );
		intermediate_result = DirectX::XMQuaternionMultiply(intermediate_result, DirectX::XMQuaternionConjugate(quaternion));
		m_cameraData.m_position = intermediate_result;
	}
	{
		DirectX::XMVECTOR intermediate_result = DirectX::XMQuaternionMultiply( quaternion, m_localForwardVector );
		intermediate_result = DirectX::XMQuaternionMultiply( intermediate_result, DirectX::XMQuaternionConjugate( quaternion ) );
		m_localForwardVector = intermediate_result;
	}
	{
		DirectX::XMVECTOR intermediate_result = DirectX::XMQuaternionMultiply( quaternion_y, m_localRightVector );
		intermediate_result = DirectX::XMQuaternionMultiply( intermediate_result, DirectX::XMQuaternionConjugate( quaternion_y ) );
		m_localRightVector = intermediate_result;
	}
	{
		DirectX::XMVECTOR intermediate_result = DirectX::XMQuaternionMultiply( quaternion, m_localUpVector );
		intermediate_result = DirectX::XMQuaternionMultiply( intermediate_result, DirectX::XMQuaternionConjugate( quaternion ) );
		m_localUpVector = intermediate_result;
	}

	m_cameraData.m_viewProjMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookToRH( m_cameraData.m_position, m_localForwardVector, m_localUpVector ) * DirectX::XMMatrixPerspectiveFovRH( m_fov, m_aspectRatio, m_nearZ, m_farZ ));
	m_cameraData.m_inverseViewProjMatrix = DirectX::XMMatrixInverse( nullptr, m_cameraData.m_viewProjMatrix );

	ResourceManager::it().setResourceNeedsCopyToGPU(m_cameraBuffer);
}

bool Camera::isAABBVisible( Mesh::AABB const& aabb ) const
{
	// Very naive visibility test, just check if any one corner of the AABB is in front of the camera
	DirectX::XMVECTOR aabbCorners[ 8 ] = {
		DirectX::XMVectorSet( aabb.m_minBounds.m128_f32[ 0 ], aabb.m_minBounds.m128_f32[ 1 ], aabb.m_minBounds.m128_f32[ 2 ], 1.0 ),
		DirectX::XMVectorSet( aabb.m_maxBounds.m128_f32[ 0 ], aabb.m_minBounds.m128_f32[ 1 ], aabb.m_minBounds.m128_f32[ 2 ], 1.0 ),
		DirectX::XMVectorSet( aabb.m_minBounds.m128_f32[ 0 ], aabb.m_maxBounds.m128_f32[ 1 ], aabb.m_minBounds.m128_f32[ 2 ], 1.0 ),
		DirectX::XMVectorSet( aabb.m_maxBounds.m128_f32[ 0 ], aabb.m_maxBounds.m128_f32[ 1 ], aabb.m_minBounds.m128_f32[ 2 ], 1.0 ),
		DirectX::XMVectorSet( aabb.m_minBounds.m128_f32[ 0 ], aabb.m_minBounds.m128_f32[ 1 ], aabb.m_maxBounds.m128_f32[ 2 ], 1.0 ),
		DirectX::XMVectorSet( aabb.m_maxBounds.m128_f32[ 0 ], aabb.m_minBounds.m128_f32[ 1 ], aabb.m_maxBounds.m128_f32[ 2 ], 1.0 ),
		DirectX::XMVectorSet( aabb.m_minBounds.m128_f32[ 0 ], aabb.m_maxBounds.m128_f32[ 1 ], aabb.m_maxBounds.m128_f32[ 2 ], 1.0 ),
		DirectX::XMVectorSet( aabb.m_maxBounds.m128_f32[ 0 ], aabb.m_maxBounds.m128_f32[ 1 ], aabb.m_maxBounds.m128_f32[ 2 ], 1.0 ),
	};

	bool visible = false;
	for ( size_t i = 0; i < 8; i++ )
	{
		visible = visible || DirectX::XMVector3Dot( DirectX::XMVector3Normalize( DirectX::XMVectorSubtract( aabbCorners[i], m_cameraData.m_position ) ), m_localForwardVector ).m128_f32[ 0 ] > 0.0f;
	}
	return visible;
}

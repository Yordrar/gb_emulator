#pragma once

#include <resource/Resource.h>
#include <geometry/Mesh.h>

class Camera
{
public:
	struct Frustum
	{
		DirectX::XMVECTOR m_cornerPositions[8];
		DirectX::XMVECTOR m_planes[6]; // Left Right Top Bottom Near Far
	};

	Camera( wchar_t const* name, DirectX::XMVECTOR position, float fov = 90.0f, float aspect_ratio = 16.0f/9.0f, float nearZ = 0.1f, float farZ = 10000.0f );
	~Camera() = default;

	void move( float delta_x, float delta_y, float delta_z );
	void rotate( float delta_angles_x, float delta_angles_y );

	bool isAABBVisible( Mesh::AABB const& aabb ) const;

	ResourceHandle getGPUBufferResource() const { return m_cameraBuffer; }
	Descriptor getCameraBufferDescriptor() const { return m_cameraBufferDescriptor; }

	struct
	{
		DirectX::XMMATRIX m_viewProjMatrix;
		DirectX::XMMATRIX m_inverseViewProjMatrix;
		DirectX::XMVECTOR m_position;
	} m_cameraData;
	
	ResourceHandle m_cameraBuffer;
	Descriptor m_cameraBufferDescriptor;

	std::wstring m_name;
	DirectX::XMVECTOR m_localRightVector;
	DirectX::XMVECTOR m_localUpVector;
	DirectX::XMVECTOR m_localForwardVector;
	float m_aspectRatio;
	float m_fov;
	float m_nearZ;
	float m_farZ;
};


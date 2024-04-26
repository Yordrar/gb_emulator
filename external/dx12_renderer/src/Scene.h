#pragma once

#include <d3d12.h>

#include <Camera.h>
#include <geometry/Mesh.h>

class Scene
{
public:
    Scene( wchar_t const* name );
    ~Scene();

    std::wstring const& getName() const { return m_name; }

    Camera& getCamera() { return m_camera; }
    ResourceHandle getCameraBufferResource() const { return m_camera.getGPUBufferResource(); }

    void addMesh( Mesh* geometry );
    std::vector< std::shared_ptr<Mesh> > const& getMeshes();

private:
    std::wstring m_name;
    Camera m_camera;

    std::vector< std::shared_ptr<Mesh> > m_meshes;
    bool meshesNeedSorting;
};


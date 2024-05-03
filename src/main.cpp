// Windows
#include <Windows.h>
#include <Windowsx.h>
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12
#include <d3d12.h>
#include <d3dx12/d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

// Std
#include <string>
#include <chrono>

// Renderer
#include <Utils.h>
#include <Window.h>
#include <Renderer.h>
#include <RenderPass.h>
#include <ComputePass.h>
#include <Scene.h>
#include <geometry/Mesh.h>
#include <resource/ResourceManager.h>
#include <geometry/MaterialManager.h>

#include <imgui/imgui.h>

// Emulator
#include "Emulator.h"
#include "Memory.h"

struct Vertex
{
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT2 m_uvs;
};

uint8_t frameTextureData[160*144*4] = {0};
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nShowCmd)
{
    unsigned int windowScale = 5;

    Window window(hInstance,
        hPrevInstance,
        pCmdLine,
        nShowCmd,
        "Game Boy Emulator",
        160 * windowScale,
        144 * windowScale);

    Renderer* renderer = window.getRenderer();

    D3D12_SUBRESOURCE_DATA frameTextureSubresData =
    {
        .pData = frameTextureData,
        .RowPitch = 160 * 4,
        .SlicePitch = 0,
    };
    ResourceHandle frameTexture = ResourceManager::it().createResource(L"frameTexture", CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 160, 140, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE), frameTextureSubresData);
    ResourceHandle mainDepthStencilTarget = ResourceManager::it().createResource(L"mainDepthStencilTarget", CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, renderer->getClientWidth(), renderer->getClientHeight(), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE));
    
    RenderPass mainPass(L"Main Pass", L"main", Descriptor(), mainDepthStencilTarget.getDefaultDepthStencilView(D3D12_DSV_FLAG_READ_ONLY_DEPTH));
    mainPass.setClearRenderTargetsBeforeRendering(true);

    ResourceManager::it().createSampler(L"globalSampler");

    std::unique_ptr<Scene> scene = std::make_unique<Scene>(L"mainScene");
    DXGI_FORMAT rtformats[8] = { DXGI_FORMAT_UNKNOWN };
    rtformats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    Material::Technique mainTechnique =
    {
        .m_name = L"main",
        .m_vertexShaderFilename = L"shader/screen.hlsl",
        .m_pixelShaderFilename = L"shader/screen.hlsl",
        .m_rtFormats = CD3DX12_RT_FORMAT_ARRAY{ rtformats, 1 },
        .m_dsFormat = DXGI_FORMAT_D32_FLOAT,
    };
    mainTechnique.m_rasterizerState.FrontCounterClockwise = false;
    mainTechnique.m_depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    mainTechnique.m_depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    Material::MaterialDesc fstMaterialDesc;
    fstMaterialDesc.m_name = StrToWideStr("FSTMaterial");
    fstMaterialDesc.m_techniques.push_back(mainTechnique);
    fstMaterialDesc.m_resourceViews.push_back(frameTexture.getDefaultShaderResourceView());
    fstMaterialDesc.m_inputLayout.push_back(D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    fstMaterialDesc.m_inputLayout.push_back(D3D12_INPUT_ELEMENT_DESC{ "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    
    MaterialManager::it().createMaterial(fstMaterialDesc);
    Mesh* fstMesh = new Mesh(StrToWideStr("fullscreen_triangle").c_str(), L"FSTMaterial");
    std::vector<Vertex> vertexBuffer(3);
    Vertex v1;
    v1.m_position.x = -1;
    v1.m_position.y = 1;
    v1.m_position.z = 0;
    v1.m_uvs.x = 0;
    v1.m_uvs.y = 0;
    Vertex v2;
    v2.m_position.x = 3;
    v2.m_position.y = 1;
    v2.m_position.z = 0;
    v2.m_uvs.x = 2;
    v2.m_uvs.y = 0;
    Vertex v3;
    v3.m_position.x = -1;
    v3.m_position.y = -3;
    v3.m_position.z = 0;
    v3.m_uvs.x = 0;
    v3.m_uvs.y = 2;
    vertexBuffer.push_back(v1);
    vertexBuffer.push_back(v2);
    vertexBuffer.push_back(v3);
    fstMesh->setVertexBuffer(vertexBuffer.data(), sizeof(Vertex), static_cast<UINT>(vertexBuffer.size()));
    scene->addMesh(fstMesh);

    Emulator emulator(frameTexture, frameTextureData);
    if (lstrcmpW(pCmdLine, L"") != 0)
    {
        emulator.openCartridgeFile(WideStrToStr(pCmdLine).c_str());
    }
    else
    {
        emulator.openCartridgeFile("bgbtest.gb");
    }

    // TODO implement UI for loading ROMs
    bool show_window = true;
    renderer->registerImguiCallback([&show_window, &renderer, &mainPass, &emulator]()
        {
            if (show_window)
            {
                Emulator::CartridgeInfo cartInfo = emulator.getCartridgeInfo();
                ImGui::Begin("Cartridge Info", &show_window);
                ImGui::Text("Name: %s", cartInfo.m_name.c_str());
                ImGui::Text("Type: 0x%x", cartInfo.m_type);
                ImGui::End();
            }
        });

    window.onKeyboardButtonDown([&emulator](WPARAM wParam, LPARAM lParam)
        {
            emulator.processInput(wParam, lParam);
        });
    window.onKeyboardButtonUp([&emulator](WPARAM wParam, LPARAM lParam)
        {
            emulator.processInput(wParam, lParam);
        });

    while (!window.shouldCloseWindow())
    {
        emulator.emulate();

        if (ResourceManager::it().getResourceNeedsCopyToGPU(frameTexture))
        {
            renderer->beginFrame();
            renderer->submitRenderPass(mainPass, *scene, { &scene->getCamera() });
            renderer->submitImGui();
            renderer->endFrame();
        }
    }

    renderer->waitForIdleGPU();

    return 0;
}
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

struct Vertex
{
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT2 m_uvs;
};

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

    std::unique_ptr<Scene> scene = std::make_unique<Scene>(L"mainScene");

    ResourceHandle mainDepthStencilTarget = ResourceManager::it().createResource(L"mainDepthStencilTarget", CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, renderer->getClientWidth(), renderer->getClientHeight(), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE));
    
    RenderPass mainPass(L"Main Pass", L"main", Descriptor(), mainDepthStencilTarget.getDefaultDepthStencilView(D3D12_DSV_FLAG_READ_ONLY_DEPTH));

    mainPass.setClearRenderTargetsBeforeRendering(true);

    ResourceManager::it().createSampler(L"globalSampler");

    Emulator emulator;
    if (lstrcmpW(pCmdLine, L"") != 0)
    {
        emulator.openCartridgeFile(WideStrToStr(pCmdLine).c_str());
    }
    else
    {
        emulator.openCartridgeFile("pokemon_red.gb");
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
                ImGui::Text("Type: %d", cartInfo.m_type);
                ImGui::End();
            }
        });

    while (!window.shouldCloseWindow())
    {
        emulator.tick();

        renderer->beginFrame();
        renderer->submitRenderPass(mainPass, *scene, { &scene->getCamera() });
        renderer->submitImGui();
        renderer->endFrame();
    }

    renderer->waitForIdleGPU();

    return 0;
}
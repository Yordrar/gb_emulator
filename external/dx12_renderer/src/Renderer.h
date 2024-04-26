#pragma once

#include <functional>

#include <RendererConstants.h>
#include <Profiler.h>
#include <resource/Descriptor.h>

class Scene;
class Fence;
class Camera;
class RenderPass;
class ComputePass;

class Renderer
{
public:
    Renderer(HWND hWnd, RECT clientRect);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator= (const Renderer&) = delete;
    Renderer(Renderer&) = delete;
    Renderer& operator= (Renderer&) = delete;

    Renderer(Renderer&&) = default;
    Renderer& operator= (Renderer&&) = default;

    static ComPtr<ID3D12Device2> device() { return s_device; }
    static ComPtr<ID3D12RootSignature> getRootSignature() { return s_rootSignature; }

    using ImguiCallback = std::function<void( void )>;
    void registerImguiCallback( ImguiCallback const& callback ) { m_imguiUserCallback = callback; m_imguiCallbackRegistered = true; }

    void beginFrame();
    void submitRenderPass( RenderPass& pass, Scene& scene, std::vector<Camera*> const& cameras );
    void submitComputePass( ComputePass& pass );
    void submitImGui();
    void endFrame();

    void waitForIdleGPU();

    static uint64_t getGlobalFrameCounter() { return s_globalFrameCounter; }
    UINT getCurrentBackbufferIndex() const { return m_currentBackBufferIndex; }
    UINT getPreviousBackbufferIndex() const { return m_previousBackBufferIndex; }
    RECT getClientRect() const { return m_clientRect; }
    UINT getClientWidth() const { return m_clientRect.right - m_clientRect.left; }
    UINT getClientHeight() const { return m_clientRect.bottom - m_clientRect.top; }
    uint64_t getTimestampFrequency() const { return m_timestampFrequency; }
    Descriptor const getCurrentBackbufferRTV() const { return m_backBufferRTVs[getCurrentBackbufferIndex()]; }
    ResourceHandle const getCurrentBackbufferHandle() const { return m_backBufferHandles[getCurrentBackbufferIndex()]; }
    Profiler& getProfiler() { return *m_profiler; }
    double getCPUFrameTime() const { return m_cpuFrameTime; }
    double getGPUFrameTime() const { return m_gpuFrameTime; }

private:
    ComPtr<ID3D12GraphicsCommandList> getNextGraphicsCommandList();
    ComPtr<ID3D12GraphicsCommandList> getNextComputeCommandList();

    void recordImgui(ComPtr<ID3D12GraphicsCommandList> commandList);

    static uint64_t s_globalFrameCounter;

    HWND m_hWnd;
    RECT m_clientRect;

    static ComPtr<ID3D12Device2> s_device;
    static ComPtr<ID3D12RootSignature> s_rootSignature;

    UINT m_previousBackBufferIndex;
    UINT m_currentBackBufferIndex;
    uint64_t m_timestampFrequency;

    ComPtr<IDXGISwapChain4> m_swapChain;
    ComPtr<ID3D12Resource> m_backBuffers[RendererConstants::sc_numBackBuffers];
    Descriptor m_backBufferRTVs[RendererConstants::sc_numBackBuffers];
    ResourceHandle m_backBufferHandles[RendererConstants::sc_numBackBuffers];
    std::unique_ptr<Fence> m_frameFence;
    uint64_t m_frameFenceValues[ RendererConstants::sc_numBackBuffers ];

    ComPtr<ID3D12CommandQueue> m_graphicsCmdQueue;
    ComPtr<ID3D12CommandQueue> m_computeCmdQueue;

    ComPtr<ID3D12GraphicsCommandList> m_preFrameCommandList;
    using CommandListContainer = std::vector< ComPtr<ID3D12GraphicsCommandList> >;
    CommandListContainer m_graphicsCommandLists;
    unsigned int m_numGraphicsCommandListsUsed = 0;
    CommandListContainer m_computeCommandLists;
    unsigned int m_numComputeCommandListsUsed = 0;
    ComPtr<ID3D12GraphicsCommandList> m_postFrameCommandList;

    ComPtr<ID3D12CommandAllocator> m_graphicsCommandAllocators[ RendererConstants::sc_numBackBuffers ];
    ComPtr<ID3D12CommandAllocator> m_computeCommandAllocators[ RendererConstants::sc_numBackBuffers ];

    bool m_imguiCallbackRegistered;
    ImguiCallback m_imguiUserCallback;

    std::unique_ptr<Profiler> m_profiler;
    double m_cpuFrameTime;
    double m_gpuFrameTime;
};


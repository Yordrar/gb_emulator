#include "Renderer.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

#include <RendererConstants.h>
#include <Fence.h>
#include <BarrierRecorder.h>
#include <Profiler.h>
#include <RenderPass.h>
#include <ComputePass.h>
#include <resource/ResourceManager.h>
#include <resource/DescriptorHeap.h>
#include <geometry/MaterialManager.h>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 710; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

uint64_t Renderer::s_globalFrameCounter = 1;
ComPtr<ID3D12Device2> Renderer::s_device = nullptr;
ComPtr<ID3D12RootSignature> Renderer::s_rootSignature = nullptr;

Renderer::Renderer( HWND hWnd, RECT clientRect)
    : m_hWnd(hWnd)
    , m_clientRect( clientRect )
    , m_currentBackBufferIndex(0)
    , m_previousBackBufferIndex(0)
    , m_swapChain( nullptr )
    , m_frameFence( nullptr )
    , m_graphicsCmdQueue( nullptr )
    , m_computeCmdQueue( nullptr )
    , m_preFrameCommandList( nullptr )
    , m_postFrameCommandList( nullptr )
    , m_imguiCallbackRegistered( false )
    , m_profiler( nullptr )
{
    for ( int i = 0; i < RendererConstants::sc_numBackBuffers; ++i ) m_frameFenceValues[ i ] = 0;
    m_frameFenceValues[ 0 ] = 1;
    m_clientRect = clientRect;
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugInterface;
    D3D12GetDebugInterface( IID_PPV_ARGS( &debugInterface ) );
    debugInterface->EnableDebugLayer();
#endif

    // Create DXGI factory
    ComPtr<IDXGIFactory7> dxgiFactory;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));

    if (!s_device)
    {
        // Select adapter with greatest amount of memory
        int i = 0;
        ComPtr<IDXGIAdapter1> currentAdapter;
        ComPtr<IDXGIAdapter1> selectedAdapter;
        SIZE_T maxDedicatedVideoMemory = 0;
        while (SUCCEEDED(dxgiFactory->EnumAdapters1(i, &currentAdapter)))
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
            currentAdapter->GetDesc1(&dxgiAdapterDesc);
            if ((dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                SUCCEEDED(D3D12CreateDevice(currentAdapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)) &&
                dxgiAdapterDesc.DedicatedVideoMemory > maxDedicatedVideoMemory)
            {
                selectedAdapter = currentAdapter;
                maxDedicatedVideoMemory = dxgiAdapterDesc.DedicatedVideoMemory;
            }

            ++i;
        }

        // Create dx12 device
        HRESULT result = D3D12CreateDevice(selectedAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&s_device));
    }

    m_profiler = std::make_unique<Profiler>(*this);
    assert(m_profiler);
    
    // Debug break on error
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if ( SUCCEEDED( s_device.As( &pInfoQueue ) ) )
    {
        pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE );
        pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
        pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE );
    }
#endif

    // Create command queues
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdQueueDesc.NodeMask = 0;
    s_device->CreateCommandQueue( &cmdQueueDesc, IID_PPV_ARGS( &m_graphicsCmdQueue ) );
    m_graphicsCmdQueue->SetName( L"Graphics Queue" );

    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    s_device->CreateCommandQueue( &cmdQueueDesc, IID_PPV_ARGS( &m_computeCmdQueue ) );
    m_computeCmdQueue->SetName( L"Compute Queue" );

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = getClientWidth();
    swapChainDesc.Height = getClientHeight();
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = RendererConstants::sc_numBackBuffers;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    ComPtr<IDXGISwapChain1> swapChain;
    dxgiFactory->CreateSwapChainForHwnd( m_graphicsCmdQueue.Get(), m_hWnd, &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf() );
    swapChain.As( &m_swapChain );

    m_frameFence = std::make_unique<Fence>( L"Renderer_frameFence" );

    // Create RTVs for the backbuffers
    for ( int i = 0; i < RendererConstants::sc_numBackBuffers; ++i )
    {
        ComPtr<ID3D12Resource> backBuffer;
        m_swapChain->GetBuffer( i, IID_PPV_ARGS( &backBuffer ) );
        m_backBuffers[i] = backBuffer;
        ResourceHandle backbufferResource = ResourceManager::it().createResource( ( L"backbuffer" + std::to_wstring( i ) ).c_str(), backBuffer );
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc =
        {
            .Format = backBuffer->GetDesc().Format,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        };
        rtvDesc.Texture2D.MipSlice = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;
        m_backBufferRTVs[i] = ResourceManager::it().getRenderTargetView( backbufferResource, rtvDesc );
        m_backBufferHandles[i] = backbufferResource;
    }

    // Create root signature
    CD3DX12_ROOT_PARAMETER slotRootParameters[ 1 ] = {};

    slotRootParameters[ 0 ].InitAsConstants( 32, 0 );

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc( _countof(slotRootParameters), slotRootParameters, 0, nullptr,
                                             D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                             D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                             D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                                             D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                             D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
                                             D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS |
                                             D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
                                             D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED );

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature( &rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                              serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf() );

    if ( errorBlob != nullptr && errorBlob->GetBufferSize() != 0 )
    {
        OutputDebugStringA( (LPCSTR)errorBlob->GetBufferPointer() );
    }

    device()->CreateRootSignature( 0,
                                             serializedRootSig->GetBufferPointer(),
                                             serializedRootSig->GetBufferSize(),
                                             IID_PPV_ARGS( s_rootSignature.GetAddressOf() ) );
    s_rootSignature->SetName( L"Global Root Signature" );

    m_graphicsCmdQueue->GetTimestampFrequency( &m_timestampFrequency );

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Imgui's Platform/Renderer backend
    // Reserve the first descriptor for imgui's font texture
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
    {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    };
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    // Because it's the first descriptor, we can just get a handle to heap start to get it
    DescriptorHeap::getDescriptorHeapCbvSrvUav().addSRV( nullptr, &srvDesc );
    ImGui_ImplWin32_Init( hWnd );
    ImGui_ImplDX12_Init( device().Get(),
                         RendererConstants::sc_numBackBuffers,
                         DXGI_FORMAT_R8G8B8A8_UNORM,
                         DescriptorHeap::getDescriptorHeapCbvSrvUav().getHeap().Get(),
                         DescriptorHeap::getDescriptorHeapCbvSrvUav().getHeap().Get()->GetCPUDescriptorHandleForHeapStart(),
                         DescriptorHeap::getDescriptorHeapCbvSrvUav().getHeap().Get()->GetGPUDescriptorHandleForHeapStart() );

    for ( int i = 0; i < RendererConstants::sc_numBackBuffers; ++i )
    {
        device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_graphicsCommandAllocators[i]));
        device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_computeCommandAllocators[i]));
    }

    device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphicsCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_preFrameCommandList));
    m_preFrameCommandList->SetName(L"preFrameCommandList");
    m_preFrameCommandList->Close();

    device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphicsCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_postFrameCommandList));
    m_postFrameCommandList->SetName(L"postFrameCommandList");
    m_postFrameCommandList->Close();
}

Renderer::~Renderer()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

static auto start = std::chrono::high_resolution_clock::now();
static std::vector<RenderPass*> submittedRenderPasses;
static std::vector<ComputePass*> submittedComputePasses;
void Renderer::beginFrame()
{
    m_numGraphicsCommandListsUsed = 0;
    m_numComputeCommandListsUsed = 0;

    m_graphicsCmdQueue->GetTimestampFrequency(&m_timestampFrequency);

    submittedRenderPasses.clear();
    submittedComputePasses.clear();

    m_gpuFrameTime = 0;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

#ifdef RENDERER_DEBUG
    start = std::chrono::high_resolution_clock::now();
#endif

    ComPtr<ID3D12CommandAllocator> currentFrameCommandAllocator = m_graphicsCommandAllocators[ getCurrentBackbufferIndex() ];
    currentFrameCommandAllocator->Reset();
    m_preFrameCommandList->Reset( currentFrameCommandAllocator.Get(), nullptr );
    ResourceManager::it().copyResourcesToGPU( m_preFrameCommandList );
    m_preFrameCommandList->Close();
}

void Renderer::submitRenderPass( RenderPass& pass, Scene& scene, std::vector<Camera*> const& cameras )
{
    ComPtr<ID3D12GraphicsCommandList> commandList = getNextGraphicsCommandList();
    pass.record( *this, commandList, scene, cameras );
    commandList->Close();
    m_gpuFrameTime += pass.getExecutionTimeMilliseconds();
    submittedRenderPasses.push_back( &pass );
}

void Renderer::submitComputePass( ComputePass& pass )
{
    ComPtr<ID3D12GraphicsCommandList> commandList = getNextComputeCommandList();
    pass.record( *this, commandList );
    commandList->Close();
    m_gpuFrameTime += pass.getExecutionTimeMilliseconds();
    submittedComputePasses.push_back( &pass );
}

void Renderer::submitImGui()
{
    ComPtr<ID3D12GraphicsCommandList> commandList = getNextGraphicsCommandList();
    recordImgui(commandList);
    commandList->Close();
}

void Renderer::endFrame()
{
    m_postFrameCommandList->Reset(m_graphicsCommandAllocators[getCurrentBackbufferIndex()].Get(), nullptr);

    for (ComputePass* computePass : submittedComputePasses)
    {
        computePass->transitionResourcesForNextFrame(m_postFrameCommandList);
    }

    m_postFrameCommandList->Close();

#ifdef RENDERER_DEBUG
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );
    m_cpuFrameTime = static_cast<double>( elapsed.count() );
#endif

    m_graphicsCmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)m_preFrameCommandList.GetAddressOf());
    for (size_t i = 0; i < submittedRenderPasses.size(); i++)
    {
        for (Fence* const& fence : submittedRenderPasses[i]->getFencesToWaitOn())
        {
            fence->GPUWait(m_graphicsCmdQueue);
        }
        m_graphicsCmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)m_graphicsCommandLists[i].GetAddressOf());
        for (Fence* const& fence : submittedRenderPasses[i]->getFencesToSignal())
        {
            fence->GPUSignal(m_graphicsCmdQueue);
        }
    }
    for (size_t i = 0; i < submittedComputePasses.size(); i++)
    {
        for (Fence* const& fence : submittedComputePasses[i]->getFencesToWaitOn())
        {
            fence->GPUWait(m_computeCmdQueue);
        }
        m_computeCmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)m_computeCommandLists[i].GetAddressOf());
        for (Fence* const& fence : submittedComputePasses[i]->getFencesToSignal())
        {
            fence->GPUSignal(m_computeCmdQueue);
        }
    }
    m_graphicsCmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)m_graphicsCommandLists[m_numGraphicsCommandListsUsed-1].GetAddressOf());

    m_swapChain->Present( 0, 0 );

    // Schedule a Signal command in the queue
    uint64_t const currentFenceValue = m_frameFenceValues[ m_currentBackBufferIndex ];
    m_frameFence->GPUSignal( m_graphicsCmdQueue, currentFenceValue );

    // Update the frame index
    m_previousBackBufferIndex = m_currentBackBufferIndex;
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready
    m_frameFence->CPUWait( m_frameFenceValues[ m_currentBackBufferIndex ] );

    // Set the fence value for the next frame
    m_frameFenceValues[ m_currentBackBufferIndex ] = currentFenceValue + 1;

    s_globalFrameCounter++;
}

void Renderer::waitForIdleGPU()
{
    uint64_t const currentFenceValue = m_frameFenceValues[ m_currentBackBufferIndex ];
    m_frameFence->GPUSignal( m_graphicsCmdQueue, currentFenceValue );
    m_frameFence->CPUWait( currentFenceValue );
    m_frameFenceValues[ m_currentBackBufferIndex ]++;
}

ComPtr<ID3D12GraphicsCommandList> Renderer::getNextGraphicsCommandList()
{
    if (m_numGraphicsCommandListsUsed < m_graphicsCommandLists.size())
    {
        m_graphicsCommandLists[m_numGraphicsCommandListsUsed]->Reset(m_graphicsCommandAllocators[getCurrentBackbufferIndex()].Get(), nullptr);
    }
    else
    {
        m_graphicsCommandLists.emplace_back(nullptr);
        device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphicsCommandAllocators[getCurrentBackbufferIndex()].Get(), nullptr, IID_PPV_ARGS(&m_graphicsCommandLists.back()));
        m_graphicsCommandLists.back()->SetName((L"Graphics Command List " + std::to_wstring(m_graphicsCommandLists.size()-1)).c_str());
    }
    return m_graphicsCommandLists[m_numGraphicsCommandListsUsed++];
}

ComPtr<ID3D12GraphicsCommandList> Renderer::getNextComputeCommandList()
{
    if (m_numComputeCommandListsUsed < m_computeCommandLists.size())
    {
        m_computeCommandLists[m_numComputeCommandListsUsed]->Reset(m_computeCommandAllocators[getCurrentBackbufferIndex()].Get(), nullptr);
    }
    else
    {
        m_computeCommandLists.emplace_back(nullptr);
        device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeCommandAllocators[getCurrentBackbufferIndex()].Get(), nullptr, IID_PPV_ARGS(&m_computeCommandLists.back()));
        m_computeCommandLists.back()->SetName((L"Compute Command List " + std::to_wstring(m_computeCommandLists.size() - 1)).c_str());
    }
    return m_computeCommandLists[m_numComputeCommandListsUsed++];
}

void Renderer::recordImgui(ComPtr<ID3D12GraphicsCommandList> commandList)
{
    if ( m_imguiCallbackRegistered )
    {
        m_imguiUserCallback();
    }
    ImGui::Render();

    PIXBeginEvent(commandList.Get(), PIX_COLOR_DEFAULT, "Imgui" );

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        DescriptorHeap::getDescriptorHeapCbvSrvUav().getHeap().Get(),
    };
    commandList->SetDescriptorHeaps( _countof( descriptorHeaps ), descriptorHeaps );

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = Renderer::getCurrentBackbufferRTV().getDescriptorHandle();
    commandList->OMSetRenderTargets( 1, &rtv, false, nullptr );

    BarrierRecorder br;
    br.recordBarrierTransition(Renderer::getCurrentBackbufferHandle(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    br.submitBarriers(commandList);

    ImGui_ImplDX12_RenderDrawData( ImGui::GetDrawData(), commandList.Get() );

    br.recordBarrierTransition(Renderer::getCurrentBackbufferHandle(), D3D12_RESOURCE_STATE_PRESENT);
    br.submitBarriers(commandList);

    PIXEndEvent();
}

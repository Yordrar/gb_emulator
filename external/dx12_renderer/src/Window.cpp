#include <Window.h>

#include <Utils.h>
#include <Renderer.h>

#include <imgui/imgui.h>

Window::HandleToWindowMap Window::s_handleToWindowMap;

Window::Window(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR lpCmdLine,
    INT nShowCmd,
    char const* windowName,
    unsigned int width,
    unsigned int height)
    : m_windowClassIdentifier(0)
    , m_windowHandle(nullptr)
    , m_renderer(nullptr)
    , m_windowMsg(MSG{})
    , m_msgToCallbackMap()
{
    std::wstring windowNameWideStr = StrToWideStr(windowName);
    std::wstring windowClassNameWideStr = windowNameWideStr;

    WNDCLASSEX windowClass =
    {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = (WNDPROC)(Window::commonWindowCallback),
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = NULL,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = (HBRUSH)COLOR_WINDOW,
        .lpszMenuName = NULL,
        .lpszClassName = windowClassNameWideStr.c_str(),
        .hIconSm = NULL,
    };

    m_windowClassIdentifier = RegisterClassEx(&windowClass);
    assert(m_windowClassIdentifier > 0);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    LONG desiredClientWidth = width;
    LONG desiredClientHeight = height;
    RECT clientRect = { 0, 0, desiredClientWidth, desiredClientHeight };
    RECT windowRect = { 0, 0, desiredClientWidth, desiredClientHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
    int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    m_windowHandle = CreateWindowEx(
        NULL,
        windowClassNameWideStr.c_str(),
        windowNameWideStr.c_str(),
        WS_OVERLAPPEDWINDOW,
        windowX,
        windowY,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    assert(m_windowHandle);

    s_handleToWindowMap[m_windowHandle] = this;

    ShowWindow(m_windowHandle, nShowCmd);

    m_renderer = std::make_unique<Renderer>(m_windowHandle, clientRect);
    assert(m_renderer);
}

Window::~Window()
{
}

Renderer* Window::getRenderer() const
{
    return m_renderer.get();
}

bool Window::shouldCloseWindow() const
{
    if (PeekMessage((LPMSG)&m_windowMsg, NULL, 0, 0, PM_QS_INPUT | PM_REMOVE))
    {
        TranslateMessage(&m_windowMsg);
        DispatchMessage(&m_windowMsg);
        return m_windowMsg.message == WM_QUIT;
    }
    return false;
}

LRESULT CALLBACK Window::commonWindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window* window = s_handleToWindowMap[hwnd];
    if (window)
    {
        return window->windowCallback(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void Window::onLeftMouseButtonDown(WindowMessageCallback const& msgCallback)
{
    m_msgToCallbackMap[WM_LBUTTONDOWN] = msgCallback;
}

void Window::onLeftMouseButtonUp(WindowMessageCallback const& msgCallback)
{
    m_msgToCallbackMap[WM_LBUTTONUP] = msgCallback;
}

void Window::onMouseMove(WindowMessageCallback const& msgCallback)
{
    m_msgToCallbackMap[WM_MOUSEMOVE] = msgCallback;
}

void Window::onMouseWheel(WindowMessageCallback const& msgCallback)
{
    m_msgToCallbackMap[WM_MOUSEWHEEL] = msgCallback;
}

void Window::onKeyboardButtonDown(WindowMessageCallback const& msgCallback)
{
    m_msgToCallbackMap[WM_KEYDOWN] = msgCallback;
}

void Window::onKeyboardButtonUp(WindowMessageCallback const& msgCallback)
{
    m_msgToCallbackMap[WM_KEYUP] = msgCallback;
}

void Window::onKeyboardCharacter(WindowMessageCallback const& msgCallback)
{
    m_msgToCallbackMap[WM_CHAR] = msgCallback;
}

void Window::onInput(WindowMessageCallback const& msgCallback)
{
    m_msgToCallbackMap[WM_INPUT] = msgCallback;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT Window::windowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
    {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    if (ImGui::GetCurrentContext())
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }

    switch (message)
    {
    case WM_PAINT:
        break;
        // The default window procedure will play a system notification sound
        // when pressing the Alt+Enter if this message is not handled
    case WM_SYSCHAR:
        break;
    case WM_SIZE:
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_KEYUP:
    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_INPUT:
        if (m_msgToCallbackMap.find(message) != m_msgToCallbackMap.end())
        {
            m_msgToCallbackMap[message](wParam, lParam);
        }
        break;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

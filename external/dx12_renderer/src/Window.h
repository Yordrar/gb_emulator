#pragma once

#include <functional>

class Renderer;

class Window
{
public:
    Window(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        PWSTR lpCmdLine,
        INT nShowCmd,
        char const* windowName,
        unsigned int width,
        unsigned int height);
    ~Window();

    Renderer* getRenderer() const;

    bool shouldCloseWindow() const;

    static LRESULT CALLBACK commonWindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    using WindowMessageCallback = std::function<void(WPARAM wParam, LPARAM lParam)>;
    void onLeftMouseButtonDown(WindowMessageCallback const& msgCallback);
    void onLeftMouseButtonUp(WindowMessageCallback const& msgCallback);
    void onMouseMove(WindowMessageCallback const& msgCallback);
    void onMouseWheel(WindowMessageCallback const& msgCallback);
    void onKeyboardButtonDown(WindowMessageCallback const& msgCallback);
    void onKeyboardButtonUp(WindowMessageCallback const& msgCallback);
    void onKeyboardCharacter(WindowMessageCallback const& msgCallback);
    void onInput(WindowMessageCallback const& msgCallback);

private:
    LRESULT CALLBACK windowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    ATOM m_windowClassIdentifier;
    HWND m_windowHandle;

    std::unique_ptr<Renderer> m_renderer;

    MSG m_windowMsg;
    bool m_shouldCloseWindow = false;

    using HandleToWindowMap = std::unordered_map< HWND, Window* >;
    static HandleToWindowMap s_handleToWindowMap;

    using WindowMesssageToCallbackMap = std::unordered_map< UINT, WindowMessageCallback >;
    WindowMesssageToCallbackMap m_msgToCallbackMap;
};
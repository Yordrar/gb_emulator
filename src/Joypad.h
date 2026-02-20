#pragma once

#include <Windows.h>
#include <cstdint>
#include <thread>

class Memory;

class Joypad
{
public:
    Joypad(Memory* memory);
    ~Joypad();

    bool updateJOYPRegister();
    void processKeyboardInput(WPARAM wParam, LPARAM lParam);

    enum InputDeviceType
    {
        Keyboard,
        Controller
    };

    InputDeviceType getCurrentInputDeviceType() const { return m_currentInputDeviceType; }
    bool areShoulderButtonsBeingPressed() const { return !m_LBPressed || !m_RBPressed; }

private:
    void pollControllerInput();

    Memory* m_memory;

    uint8_t m_upPressed;
    uint8_t m_downPressed;
    uint8_t m_leftPressed;
    uint8_t m_rightPressed;
    uint8_t m_APressed;
    uint8_t m_BPressed;
    uint8_t m_startPressed;
    uint8_t m_selectPressed;
    uint8_t m_LBPressed;
    uint8_t m_RBPressed;

    std::thread m_controllerPollingThread;
    bool m_continueControllerPollingThread = true;

    InputDeviceType m_currentInputDeviceType = InputDeviceType::Keyboard;
};
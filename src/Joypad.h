#pragma once

#include <Windows.h>
#include <cstdint>

class Memory;

class Joypad
{
public:
    Joypad(Memory* memory);
    ~Joypad();

    bool updateJOYP();
    void processInput(WPARAM wParam, LPARAM lParam);

private:
    Memory* m_memory;

    uint8_t m_upPressed;
    uint8_t m_downPressed;
    uint8_t m_leftPressed;
    uint8_t m_rightPressed;
    uint8_t m_APressed;
    uint8_t m_BPressed;
    uint8_t m_startPressed;
    uint8_t m_selectPressed;
};
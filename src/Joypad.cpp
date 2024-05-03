#include "Joypad.h"

#include <Windows.h>
#include <format>

#include "Memory.h"

Joypad::Joypad(Memory* memory)
    : m_memory(memory)
    , m_upPressed(0)
    , m_downPressed(1)
    , m_leftPressed(1)
    , m_rightPressed(1)
    , m_APressed(1)
    , m_BPressed(1)
    , m_startPressed(1)
    , m_selectPressed(1)
{
}

Joypad::~Joypad()
{
}

bool Joypad::updateJOYP()
{
    uint8_t JOYP = m_memory->read(0xFF00);
    bool selectButtonKeys = !(JOYP & 0b00100000);
    bool selectDirectionKeys = !(JOYP & 0b00010000);

    uint8_t newJOYP = (JOYP & 0b11110000);

    if (selectDirectionKeys)
    {
        newJOYP |= (m_downPressed << 3) | (m_upPressed << 2) | (m_leftPressed << 1) | (m_rightPressed);
    }
    else if (selectButtonKeys)
    {
        newJOYP |= (m_startPressed << 3) | (m_selectPressed << 2) | (m_BPressed << 1) | (m_APressed);
    }
    else
    {
        newJOYP |= 0x0F;
    }

    m_memory->write(0xFF00, newJOYP | 0b11000000);

    return (JOYP & 0x0F) != (newJOYP & 0x0F);
}

void Joypad::processInput(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case VK_DOWN:
        m_downPressed = (GetKeyState(VK_DOWN) & 0x8000) >> 15;
        m_downPressed = m_downPressed ? 0 : 1;
        break;
    case VK_UP:
        m_upPressed = (GetKeyState(VK_UP) & 0x8000) >> 15;
        m_upPressed = m_upPressed ? 0 : 1;
        break;
    case VK_LEFT:
        m_leftPressed = (GetKeyState(VK_LEFT) & 0x8000) >> 15;
        m_leftPressed = m_leftPressed ? 0 : 1;
        break;
    case VK_RIGHT:
        m_rightPressed = (GetKeyState(VK_RIGHT) & 0x8000) >> 15;
        m_rightPressed = m_rightPressed ? 0 : 1;
        break;
    case 0x5A: // Z
        m_APressed = (GetKeyState(0x5A) & 0x8000) >> 15;
        m_APressed = m_APressed ? 0 : 1;
        break;
    case 0x58: // X
        m_BPressed = (GetKeyState(0x58) & 0x8000) >> 15;
        m_BPressed = m_BPressed ? 0 : 1;
        break;
    case 0x43: // C
        m_startPressed = (GetKeyState(0x43) & 0x8000) >> 15;
        m_startPressed = m_startPressed ? 0 : 1;
        break;
    case 0x56: // V
        m_selectPressed = (GetKeyState(0x56) & 0x8000) >> 15;
        m_selectPressed = m_selectPressed ? 0 : 1;
        break;
    default:
        break;
    }
}

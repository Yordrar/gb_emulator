#include "Joypad.h"

#include <Windows.h>
#include <format>

#include "Memory.h"

Joypad::Joypad(CPU* cpu, Memory* memory)
    : m_cpu(cpu)
    , m_memory(memory)
{
}

Joypad::~Joypad()
{
}

void Joypad::update(double deltaTimeSeconds)
{
    uint8_t JOYP = m_memory->read(0xFF00);
    bool selectButtonKeys = !(JOYP & 0b00100000);
    bool selectDirectionKeys = !(JOYP & 0b00010000);

    uint8_t newJOYP = (JOYP & 0b11110000) | 0b00001111;

    if (selectDirectionKeys)
    {
        uint8_t downPressed = !((GetKeyState(VK_DOWN) & 0x8000) >> 15);
        uint8_t upPressed = !((GetKeyState(VK_UP) & 0x8000) >> 15);
        uint8_t leftPressed = !((GetKeyState(VK_LEFT) & 0x8000) >> 15);
        uint8_t rightPressed = !((GetKeyState(VK_RIGHT) & 0x8000) >> 15);
        newJOYP |= (downPressed << 3) | (upPressed << 2) | (leftPressed << 1) | (rightPressed);
    }
    else if (selectButtonKeys)
    {
        uint8_t startPressed = !((GetKeyState('c') & 0x8000) >> 15);
        uint8_t selectPressed = !((GetKeyState('v') & 0x8000) >> 15);
        uint8_t buttonBPressed = !((GetKeyState('x') & 0x8000) >> 15);
        uint8_t buttonAPressed = !((GetKeyState('z') & 0x8000) >> 15);
        newJOYP |= (startPressed << 3) | (selectPressed << 2) | (buttonBPressed << 1) | (buttonAPressed);
    }

    if ((JOYP & 0x0F) != (newJOYP & 0x0F))
    {
        //OutputDebugStringA(std::format("{:x}\n", newJOYP).c_str());
    }

    m_memory->write(0xFF00, newJOYP);
}

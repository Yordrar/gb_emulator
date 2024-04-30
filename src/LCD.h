#pragma once

#include <cstdint>

#include <resource/ResourceHandle.h>

class CPU;
class Memory;

class LCD
{
public:
    LCD(CPU* cpu, Memory* memory, ResourceHandle frameTexture);
    ~LCD();

    void update(double deltaTimeSeconds);

private:
    CPU* m_cpu;
    Memory* m_memory;

    ResourceHandle m_frameTexture;

    double m_timerCounter;
    uint8_t m_currentLine;
};
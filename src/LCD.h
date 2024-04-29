#pragma once

#include <cstdint>

#include <resource/ResourceHandle.h>

class CPU;

class LCD
{
public:
    LCD(CPU* cpu, ResourceHandle frameTexture);
    ~LCD();

    void update(double deltaTimeSeconds);

private:
    CPU* m_cpu;
    ResourceHandle m_frameTexture;

    double m_timerCounter;
    uint8_t m_currentLine;
};
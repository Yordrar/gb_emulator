#pragma once

#include <cstdint>

#include <resource/ResourceHandle.h>

class CPU;
class Memory;

class LCD
{
public:
    LCD(CPU* cpu, Memory* memory, ResourceHandle frameTexture, uint8_t* frameTextureData);
    ~LCD();

    void update(double deltaTimeSeconds);

private:
    void writeScanlineToFrame();

    CPU* m_cpu;
    Memory* m_memory;

    ResourceHandle m_frameTexture;
    uint8_t* m_frameTextureData;

    double m_timerCounter;
    uint8_t m_currentLine;
};
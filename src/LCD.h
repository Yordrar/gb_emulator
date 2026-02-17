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

    void update(uint64_t cyclesToEmulate);

private:
    void writeScanlineToFrame();

    CPU* m_cpu;
    Memory* m_memory;

    ResourceHandle m_frameTexture;
    uint8_t* m_frameTextureData;

    uint8_t m_priorityMap[160 * 144] = {};
    uint8_t m_BGColorIndex[160 * 144] = {};

    double m_timerCounter;
    int m_currentLine;
};
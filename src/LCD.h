#pragma once

#include <cstdint>
#include <vector>

#include <resource/ResourceHandle.h>

class CPU;
class Memory;

class LCD
{
public:
    LCD(CPU* cpu, Memory* memory, ResourceHandle frameTexture, uint8_t* frameTextureData);
    ~LCD();

    void update(uint64_t cyclesToEmulate);

    struct RGB
    {
        uint8_t r, g, b;
    };
private:
    void clearScreen();
    void fillScanlineWithColor(uint8_t line, RGB color);
    void writeScanlineToFrame();
    void readSpritesToDraw();
    void checkForSTATInterrupt();

    CPU* m_cpu;
    Memory* m_memory;

    ResourceHandle m_frameTexture;
    uint8_t* m_frameTextureData;

    uint8_t m_priorityMap[160 * 144] = {};
    uint8_t m_BGColorIndex[160 * 144] = {};

    struct Sprite
    {
        int spriteY, spriteX, tileIndex, attributes, locationInOAM;
    };
    std::vector<Sprite> m_spritesToDraw;

    double m_timerCounter;
    uint8_t m_currentLine;

    bool m_isDisplayEnabled = false;
    bool m_skipNextFrame = false;
};
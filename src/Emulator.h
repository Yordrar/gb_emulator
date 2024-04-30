#pragma once

#include <resource/ResourceHandle.h>

#include <string>
#include <cstdint>
#include <memory>

class CPU;
class Timer;
class LCD;
class Memory;

class Emulator
{
public:
    Emulator(ResourceHandle frameTexture);
    ~Emulator();

    struct CartridgeInfo
    {
        std::string m_name;
        uint8_t m_type;
    };

    void openCartridgeFile(char const* cartridgeFilename);
    CartridgeInfo getCartridgeInfo() const { return m_cartridgeInfo; }

    void emulate();

private:
    ResourceHandle m_frameTexture;

    std::unique_ptr<uint8_t[]> m_cartridge;
    size_t m_cartridgeSize;
    CartridgeInfo m_cartridgeInfo;

    std::unique_ptr<Memory> m_memory;
    std::unique_ptr<CPU> m_cpu;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<LCD> m_lcd;
};
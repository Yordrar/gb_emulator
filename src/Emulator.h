#pragma once

#include <string>
#include <cstdint>
#include <memory>

class CPU;

class Emulator
{
public:
    Emulator();
    ~Emulator();

    struct CartridgeInfo
    {
        std::string m_name;
        uint8_t m_type;
    };

    void openCartridgeFile(char const* cartridgeFilename);
    CartridgeInfo getCartridgeInfo() const { return m_cartridgeInfo; }

    void tick();

private:
    std::unique_ptr<uint8_t[]> m_cartridge;
    size_t m_cartridgeSize;
    CartridgeInfo m_cartridgeInfo;

    std::unique_ptr<CPU> m_cpu;
};
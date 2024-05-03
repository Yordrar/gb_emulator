#include "Emulator.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>

#include "CPU.h"
#include "Timer.h"
#include "LCD.h"
#include "Memory.h"
#include "Joypad.h"

std::chrono::steady_clock::time_point start;
std::chrono::steady_clock::time_point end;
Emulator::Emulator(ResourceHandle frameTexture, uint8_t* frameTextureData)
    : m_frameTexture(frameTexture)
    , m_frameTextureData(frameTextureData)
{
    start = std::chrono::high_resolution_clock::now();
}

Emulator::~Emulator()
{
}

void Emulator::openCartridgeFile(char const* cartridgeFilename)
{
    m_cartridgeSize = std::filesystem::file_size(cartridgeFilename);
    m_cartridge = std::make_unique<uint8_t[]>(m_cartridgeSize);

    std::ifstream file(cartridgeFilename, std::ios_base::binary);
    file.read(reinterpret_cast<char*>(m_cartridge.get()), m_cartridgeSize);

    m_cartridgeInfo =
    {
        .m_name = std::string(reinterpret_cast<char*>(&m_cartridge[0x134])),
        .m_type = m_cartridge[0x147],
    };

    m_memory = std::make_unique<Memory>(m_cartridge.get(), m_cartridgeSize);
    m_cpu = std::make_unique<CPU>(m_memory.get());
    m_timer = std::make_unique<Timer>(m_cpu.get(), m_memory.get());
    m_lcd = std::make_unique<LCD>(m_cpu.get(), m_memory.get(), m_frameTexture, m_frameTextureData);
    m_joypad = std::make_unique<Joypad>(m_cpu.get(), m_memory.get());
}

void Emulator::emulate()
{
    end = std::chrono::high_resolution_clock::now();

    auto elapsedMilliseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double deltaTimeSeconds = std::chrono::abs(elapsedMilliseconds).count() / 1000000000.0;

    m_joypad->update(deltaTimeSeconds);
    m_timer->update(deltaTimeSeconds);
    m_lcd->update(deltaTimeSeconds);
    m_cpu->update(deltaTimeSeconds);

    start = std::chrono::high_resolution_clock::now();
}

#include "Emulator.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>

#include "CPU.h"
#include "Timer.h"
#include "LCD.h"

std::chrono::steady_clock::time_point start;
std::chrono::steady_clock::time_point end;
Emulator::Emulator(ResourceHandle frameTexture)
    : m_frameTexture(frameTexture)
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

    m_cpu = std::make_unique<CPU>(m_cartridge.get(), m_cartridgeSize);
    m_timer = std::make_unique<Timer>(m_cpu.get());
    m_lcd = std::make_unique<LCD>(m_cpu.get(), m_frameTexture);
}

void Emulator::emulate()
{
    end = std::chrono::high_resolution_clock::now();

    auto elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double deltaTimeSeconds = std::chrono::abs(elapsedMilliseconds).count() / 1000.0;

    m_timer->update(deltaTimeSeconds);
    m_lcd->update(deltaTimeSeconds);
    m_cpu->update(deltaTimeSeconds);

    start = std::chrono::high_resolution_clock::now();
}

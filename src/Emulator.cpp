#include "Emulator.h"

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>

#include "CPU.h"

std::chrono::steady_clock::time_point start;
std::chrono::steady_clock::time_point end;
Emulator::Emulator()
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

    m_cpu = std::make_unique<CPU>(m_cartridge.get());
}

static uint64_t executedTicks = 0;
void Emulator::tick()
{
    end = std::chrono::high_resolution_clock::now();

    auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    uint64_t elapsedTicks = CPU::FrequencyHz * std::chrono::abs(elapsedSeconds).count();

    while (executedTicks < elapsedTicks)
    {
        executedTicks += m_cpu->executeNextInstruction();
    }

    executedTicks = executedTicks - elapsedTicks;

    start = std::chrono::high_resolution_clock::now();
}

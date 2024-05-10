#include "Emulator.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>
#include <format>

#include "CPU.h"
#include "Timer.h"
#include "LCD.h"
#include "Memory.h"
#include "Joypad.h"
#include "Sound.h"

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
    if (m_cartridgeInfo.m_type == 0x13)
    {
        saveBatteryBackedRamToFile();
    }
}

void Emulator::openRomFile(char const* romFilename)
{
    std::string filepath(romFilename);
    if (filepath.starts_with('\"'))
    {
        filepath = filepath.substr(1);
    }
    if (filepath.ends_with('\"'))
    {
        filepath = filepath.substr(0, filepath.size()-1);
    }

    romFilename = filepath.data();

    if (!std::filesystem::exists(romFilename))
    {
        return;
    }

    saveBatteryBackedRamToFile();

    m_romFilename = romFilename;

    m_cartridgeSize = std::filesystem::file_size(romFilename);
    m_cartridge = std::make_unique<uint8_t[]>(m_cartridgeSize);

    std::ifstream file(romFilename, std::fstream::in | std::fstream::binary);
    file.read(reinterpret_cast<char*>(m_cartridge.get()), m_cartridgeSize);

    m_cartridgeInfo =
    {
        .m_name = std::string(reinterpret_cast<char*>(&m_cartridge[0x134])),
        .m_type = m_cartridge[0x147],
    };

    m_memory = std::make_unique<Memory>(m_cartridge.get(), m_cartridgeSize);
    m_joypad = std::make_unique<Joypad>(m_memory.get());
    m_sound = std::make_unique<Sound>(m_memory.get());
    m_cpu = std::make_unique<CPU>(m_memory.get(), m_joypad.get());
    m_timer = std::make_unique<Timer>(m_cpu.get(), m_memory.get());
    m_lcd = std::make_unique<LCD>(m_cpu.get(), m_memory.get(), m_frameTexture, m_frameTextureData);

    loadSavFileToRam();
}

static int a = 0;
void Emulator::emulate()
{
    if (!m_cartridge) return;

    end = std::chrono::high_resolution_clock::now();

    auto elapsedNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double deltaTimeSeconds = std::chrono::abs(elapsedNanoseconds).count() / 1000000000.0;

    if (deltaTimeSeconds != 0.0)
    {
        m_memory->updateRTC(deltaTimeSeconds);
    }

    uint64_t executedCycles = m_cpu->executeInstruction();
    m_sound->update(executedCycles);
    m_timer->update(executedCycles);
    m_lcd->update(executedCycles);

    start = std::chrono::high_resolution_clock::now();
}

void Emulator::processInput(WPARAM wParam, LPARAM lParam)
{
    if (!m_cartridge) return;

    m_joypad->processInput(wParam, lParam);
}

void Emulator::saveBatteryBackedRamToFile()
{
    if (!m_cartridge) return;

    std::string savFilename = m_romFilename.substr(0, m_romFilename.size() - 2) + "sav";
    std::ofstream file(savFilename, std::fstream::out | std::fstream::binary | std::fstream::trunc);
    m_memory->saveRamBanksToFile(file);
}

void Emulator::loadSavFileToRam()
{
    if (!m_cartridge) return;

    std::string savFilename = m_romFilename.substr(0, m_romFilename.size() - 2) + "sav";

    if (!std::filesystem::exists(savFilename))
    {
        return;
    }

    std::ifstream file(savFilename, std::fstream::in | std::fstream::binary);
    m_memory->loadRamBanksFromFile(file);
}

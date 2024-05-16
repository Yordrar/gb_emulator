#include "Emulator.h"

#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>
#include <cassert>

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
    saveBatteryBackedRamToFile();
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

    saveBatteryBackedRamToFile(); // Save ram to file in case we had another game opened before this

    m_romFilename = romFilename;

    m_cartridgeSize = std::filesystem::file_size(romFilename);
    m_cartridge = std::make_unique<uint8_t[]>(m_cartridgeSize);

    std::ifstream file(romFilename, std::fstream::in | std::fstream::binary);
    file.read(reinterpret_cast<char*>(m_cartridge.get()), m_cartridgeSize);

    extractCartridgeInfo();

    if (m_cartridgeInfo.hasMBC3())
    {
        m_memory = std::make_unique<MBC3>(m_cartridge.get(), m_cartridgeSize);
    }
    else if (m_cartridgeInfo.hasMBC5())
    {
        m_memory = std::make_unique<MBC5>(m_cartridge.get(), m_cartridgeSize);
    }
    else
    {
        m_memory = std::make_unique<Memory>(m_cartridge.get(), m_cartridgeSize);
    }
    m_joypad = std::make_unique<Joypad>(m_memory.get());
    m_sound = std::make_unique<Sound>(m_memory.get());
    m_cpu = std::make_unique<CPU>(m_memory.get(), m_joypad.get());
    m_timer = std::make_unique<Timer>(m_cpu.get(), m_memory.get());
    m_lcd = std::make_unique<LCD>(m_cpu.get(), m_memory.get(), m_frameTexture, m_frameTextureData);

    loadSavFileToRam();
}

static double saveTimer = 0.0;
void Emulator::emulate()
{
    if (!m_cartridge) return;

    end = std::chrono::high_resolution_clock::now();

    auto elapsedNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double deltaTimeSeconds = std::chrono::abs(elapsedNanoseconds).count() / 1000000000.0;

    saveTimer += deltaTimeSeconds;

    if (deltaTimeSeconds != 0.0)
    {
        m_memory->updateRTC(deltaTimeSeconds);
    }

    uint64_t executedCycles = m_cpu->executeInstruction();
    m_sound->update(executedCycles);
    m_timer->update(executedCycles);
    m_lcd->update(executedCycles);

    if (saveTimer >= 1.0)
    {
        saveTimer = 0.0;
        if (m_memory->areRamBanksDirty())
        {
            saveBatteryBackedRamToFile();
        }
    }

    start = std::chrono::high_resolution_clock::now();
}

void Emulator::processKeyboardInput(WPARAM wParam, LPARAM lParam)
{
    if (!m_cartridge) return;

    m_joypad->processKeyboardInput(wParam, lParam);
}

void Emulator::saveBatteryBackedRamToFile()
{
    if (!m_cartridge || !m_cartridgeInfo.hasBatteryBackedRam()) return;

    std::string savFilename = m_romFilename.substr(0, m_romFilename.rfind(".") + 1) + "sav";
    std::ofstream file(savFilename, std::fstream::out | std::fstream::binary | std::fstream::trunc);
    m_memory->saveRamBanksToFile(file);
}

void Emulator::loadSavFileToRam()
{
    if (!m_cartridge) return;

    std::string savFilename = m_romFilename.substr(0, m_romFilename.rfind(".") + 1) + "sav";

    if (!std::filesystem::exists(savFilename))
    {
        return;
    }

    std::ifstream file(savFilename, std::fstream::in | std::fstream::binary);
    m_memory->loadRamBanksFromFile(file);
}

void Emulator::extractCartridgeInfo()
{
    uint64_t numRomBanks = 0;
    uint64_t ramSize = 0;
    uint64_t numRamBanks = 0;

    switch (m_cartridge[0x148])
    {
    case 0x52:
        numRomBanks = 72;
        break;
    case 0x53:
        numRomBanks = 80;
        break;
    case 0x54:
        numRomBanks = 96;
        break;
    default:
        numRomBanks = 2 * static_cast<uint64_t>(std::powf(2.0f, m_cartridge[0x148]));
        break;
    }

    switch (m_cartridge[0x149])
    {
    case 0:
        ramSize = 0;
        numRamBanks = 0;
        break;
    case 1:
        ramSize = 2 * 1024;
        numRamBanks = 1;
        break;
    case 2:
        ramSize = 8 *1024;
        numRamBanks = 1;
        break;
    case 3:
        ramSize = 32 * 1024;
        numRamBanks = 4;
        break;
    case 4:
        ramSize = 128 * 1024;
        numRamBanks = 16;
    default:
        assert(false);
        break;
    }

    m_cartridgeInfo =
    {
        .m_name = std::string(reinterpret_cast<char*>(&m_cartridge[0x134])),
        .m_isColorGB = m_cartridge[0x143] == 0x80,
        .m_hasSGBFunctions = m_cartridge[0x146] == 0x03,
        .m_type = m_cartridge[0x147],
        .m_romSize = numRomBanks * 16 * 1024,
        .m_numRomBanks = numRomBanks,
        .m_ramSize = ramSize,
        .m_numRamBanks = numRamBanks,
    };   
}

std::string Emulator::CartridgeInfo::getCartridgeTypeStr() const
{
    switch (m_type)
    {
    case 0x00:
        return "ROM ONLY";
        break;
    case 0x01:
        return "ROM+MBC1";
        break;
    case 0x02:
        return "ROM+MBC1+RAM";
        break;
    case 0x03:
        return "ROM+MBC1+RAM+BATTERY";
        break;
    case 0x05:
        return "ROM+MBC2";
        break;
    case 0x06:
        return "ROM+MBC2+BATTERY";
        break;
    case 0x08:
        return "ROM+RAM";
        break;
    case 0x09:
        return "ROM+RAM+BATTERY";
        break;
    case 0x0B:
        return "ROM+MMM01";
        break;
    case 0x0C:
        return "ROM+MMM01+SRAM";
        break;
    case 0x0D:
        return "ROM+MMM01+SRAM+BATTERY";
        break;
    case 0x0F:
        return "ROM+MBC3+TIMER+BATTERY";
        break;
    case 0x10:
        return "ROM+MBC3+TIMER+RAM+BATTERY";
        break;
    case 0x11:
        return "ROM+MBC3";
        break;
    case 0x12:
        return "ROM+MBC3+RAM";
        break;
    case 0x13:
        return "ROM+MBC3+RAM+BATTERY";
        break;
    case 0x19:
        return "ROM+MBC5";
        break;
    case 0x1A:
        return "ROM+MBC5+RAM";
        break;
    case 0x1B:
        return "ROM+MBC5+RAM+BATTERY";
        break;
    case 0x1C:
        return "ROM+MBC5+RUMBLE";
        break;
    case 0x1D:
        return "ROM+MBC5+RUMBLE+SRAM";
        break;
    case 0x1E:
        return "ROM+MBC5+RUMBLE+SRAM+BATTERY";
        break;
    case 0x20:
        return "MBC6";
        break;
    case 0x22:
        return "MBC7+SENSOR+RUMBLE+RAM+BATTERY";
        break;
    case 0xFC:
        return "Pocket Camera";
        break;
    case 0xFD:
        return "Bandai TAMA5";
        break;
    case 0xFE:
        return "Hudson HuC-3";
        break;
    case 0xFF:
        return "Hudson HuC-1+RAM+BATTERY";
        break;
    default:
        return "Unrecognized cartridge type";
        break;
    }
}

bool Emulator::CartridgeInfo::hasBatteryBackedRam() const
{
    return m_type == 0x03 ||
        m_type == 0x06 ||
        m_type == 0x09 ||
        m_type == 0x0D ||
        m_type == 0x0F ||
        m_type == 0x10 ||
        m_type == 0x13 ||
        m_type == 0x1B ||
        m_type == 0x1E ||
        m_type == 0x22 ||
        m_type == 0xFF;
}

bool Emulator::CartridgeInfo::hasMBC1() const
{
    return m_type == 0x01 ||
        m_type == 0x02 ||
        m_type == 0x03;
}

bool Emulator::CartridgeInfo::hasMBC2() const
{
    return m_type == 0x05 ||
        m_type == 0x06;
}

bool Emulator::CartridgeInfo::hasMBC3() const
{
    return m_type == 0x10 ||
        m_type == 0x11 ||
        m_type == 0x12 ||
        m_type == 0x13;
}

bool Emulator::CartridgeInfo::hasMBC5() const
{
    return m_type == 0x19 ||
        m_type == 0x1A ||
        m_type == 0x1B ||
        m_type == 0x1C ||
        m_type == 0x1D ||
        m_type == 0x1E;
}

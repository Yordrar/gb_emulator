#include "Emulator.h"

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

#include "CPU.h"

Emulator::Emulator()
{
}

Emulator::~Emulator()
{
}

void Emulator::openCartridgeFile(char const* cartridgeFilename)
{
    size_t fileSize = std::filesystem::file_size(cartridgeFilename);
    m_cartridge = std::make_unique<uint8_t[]>(fileSize);

    std::ifstream file(cartridgeFilename, std::ios_base::binary);
    file.read(reinterpret_cast<char*>(m_cartridge.get()), fileSize);

    m_cartridgeInfo =
    {
        .m_name = std::string(reinterpret_cast<char*>(&m_cartridge[0x134])),
        .m_type = m_cartridge[0x147],
    };

    m_cpu = std::make_unique<CPU>(m_cartridge.get());
}

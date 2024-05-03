#include "Memory.h"

#include <Windows.h>

#include <string>
#include <algorithm>

Memory::Memory(uint8_t* cartridge, size_t cartridgeSize)
    : m_cartridge(cartridge)
    , m_cartridgeSize(cartridgeSize)
    , m_currentRomBank(1)
{
    std::memcpy(m_memory, m_cartridge, std::min(0x3FFF, static_cast<int>(m_cartridgeSize)));
}

Memory::~Memory()
{
}

uint8_t& Memory::read(size_t address)
{
    // Reading from ROM bank
    if (address >= 0x4000 && address <= 0x7FFF)
    {
        return m_cartridge[(m_currentRomBank * 0x4000) + (address - 0x4000)];
    }

    // Reading from RAM bank
    if (address >= 0xA000 && address <= 0xBFFF)
    {
        switch (m_currentRamBank)
        {
        case 0:
        {
            return m_ramBank0[(address - 0xA000)];
            break;
        }
        case 1:
        {
            return m_ramBank1[(address - 0xA000)];
            break;
        }
        case 2:
        {
            return m_ramBank2[(address - 0xA000)];
            break;
        }
        case 3:
        {
            return m_ramBank3[(address - 0xA000)];
            break;
        }
        }
    }

    // Reading from Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    return m_memory[address];
}

void Memory::write(size_t address, uint8_t value)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        if ((value & 0x0F) == 0x0A)
        {
            // TODO enable RAM
        }
        else
        {
            // TODO disable RAM
        }
        return;
    }

    if (address >= 0x2000 && address <= 0x3FFF)
    {
        uint8_t selectedRomBank = (value & 0x7F);
        if (selectedRomBank == 0)
        {
            selectedRomBank++;
        }
        m_currentRomBank = selectedRomBank;
        return;
    }

    if (address >= 0x4000 && address <= 0x5FFF)
    {
        m_currentRamBank = (value & 3);
        return;
    }

    if (address >= 0x6000 && address <= 0x7FFF)
    {
        m_currentBankingMode = (value & 1);
        return;
    }

    // Writing to RAM bank
    if (address >= 0xA000 && address <= 0xBFFF)
    {
        switch (m_currentRamBank)
        {
        case 0:
        {
            m_ramBank0[(address - 0xA000)] = value;
            break;
        }
        case 1:
        {
            m_ramBank1[(address - 0xA000)] = value;
            break;
        }
        case 2:
        {
            m_ramBank2[(address - 0xA000)] = value;
            break;
        }
        case 3:
        {
            m_ramBank3[(address - 0xA000)] = value;
            break;
        }
        }
    }

    // Writing to Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    //Joypad
    if (address == 0xFF00)
    {
        uint8_t JOYP = read(0xFF00);
        uint8_t newJOYP = (JOYP & 0b11001111) | value;
        value = newJOYP;
    }

    // OAM DMA transfer
    if (address == 0xFF46)
    {
        uint16_t sourceAddress = (uint16_t(std::clamp(static_cast<unsigned int>(value), 0u, 0xF1u)) << 8);
        for (int i = 0; i <= 0x9F; i++)
        {
            write(0xFE00 | i, read(sourceAddress | i));
        }
    }

    m_memory[address] = value;
}
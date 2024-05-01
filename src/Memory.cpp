#include "Memory.h"

#include <string>

Memory::Memory(uint8_t* cartridge, size_t cartridgeSize)
    : m_cartridge(cartridge)
    , m_cartridgeSize(cartridgeSize)
    , m_currentRomBank(1)
{
    std::memcpy(m_memory, m_cartridge, std::min(0x7FFF, static_cast<int>(m_cartridgeSize)));
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

    // Writing to background tile map
    if (address >= 0x9800 && address <= 0x9BFF)
    {
        uint8_t paletteColors = read(0xFF47);
        uint16_t tileIndex = static_cast<uint16_t>(address) - 0x9800;
        uint16_t beginTileData = 0x8000 + (16 * value);
        uint16_t tileLocationInBackground = ((tileIndex / 32) * 256 * 8) + ((tileIndex % 32) * 8);
        for (int i = 0; i < 16; i+=2)
        {
            uint8_t tileLSB = read(beginTileData + i);
            uint8_t tileMSB = read(beginTileData + i + 1);
            for (int j = 7; j >= 0; j--)
            {
                uint8_t paletteColorIdx = (((tileMSB & (1 << j)) >> j) << 1) | (((tileLSB & (1 << j)) >> j));
                uint8_t color = (paletteColors >> (paletteColorIdx * 2)) & 0x3;
                uint8_t tileOffset = ((i / 2) * 256) + abs(j - 7);
                switch (color)
                {
                case 0:
                {
                    m_background[tileLocationInBackground + tileOffset] = 0.0f;
                    break;
                }
                case 1:
                {
                    m_background[tileLocationInBackground + tileOffset] = 0.33f;
                    break;
                }
                case 2:
                {
                    m_background[tileLocationInBackground + tileOffset] = 0.66f;
                    break;
                }
                case 3:
                {
                    m_background[tileLocationInBackground + tileOffset] = 1.0f;
                    break;
                }
                }
            }
        }
    }

    // Writing to window tile map
    if (address >= 0x9C00 && address <= 0x9FFF)
    {

    }

    // Writing to Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    m_memory[address] = value;
}
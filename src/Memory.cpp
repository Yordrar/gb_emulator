#include "Memory.h"

#include <Windows.h>

#include <algorithm>

Memory::Memory(uint8_t* cartridge, size_t cartridgeSize)
    : m_cartridge(cartridge)
    , m_cartridgeSize(cartridgeSize)
{
    std::memcpy(m_memory, m_cartridge, std::min(0x7FFF, static_cast<int>(m_cartridgeSize)));
}

Memory::~Memory()
{
}

void Memory::updateRTC(double deltaTimeSeconds)
{
    m_rtcCounter += deltaTimeSeconds;

    if (m_rtcCounter >= 1.0)
    {
        m_rtcCounter = 0;
        m_rtcSeconds++;
        if (m_rtcSeconds >= 60)
        {
            m_rtcSeconds = 0;
            m_rtcMinutes++;
            if (m_rtcMinutes >= 60)
            {
                m_rtcMinutes = 0;
                m_rtcHours++;
                if (m_rtcHours >= 24)
                {
                    m_rtcHours = 0;
                    m_rtcLowerDayCounter++;
                }
            }
        }
    }
}

uint8_t Memory::read(size_t address)
{
    // Reading from ROM bank
    if (address >= 0x4000 && address <= 0x7FFF)
    {
        return m_cartridge[(m_currentRomBank * 0x4000) + (address - 0x4000)];
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
    // ROM bank number
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

    // Writing to Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    //Enable/Disable APU
    if (address == 0xFF26)
    {
        bool newEnabled = (value >> 7) != 0;
        bool wasEnabled = (m_memory[0xFF26] >> 7) != 0;
        if (!newEnabled && wasEnabled)
        {
            for (uint16_t address = 0xFF10; address <= 0xFF25; address++)
            {
                m_memory[address] = 0;
            }
        }
        m_memory[0xFF26] = (m_memory[0xFF26] & 0x0F) | value;
        return;
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

MBC1::MBC1(uint8_t* cartridge, size_t cartridgeSize)
    : Memory(cartridge, cartridgeSize)
{
}

MBC1::~MBC1()
{
}

uint8_t MBC1::read(size_t address)
{
    // Reading from ROM bank 0
    if (address >= 0x0000 && address <= 0x3FFF)
    {
        if (m_currentBankingMode == 1)
        {
            return m_cartridge[(m_currentRomBank * 0x4000) + address];
        }
    }

    // Reading from ROM bank #
    if (address >= 0x4000 && address <= 0x7FFF)
    {
        if (m_currentBankingMode == 0 && (m_currentRomBank & 0x1F) == 0)
        {
            return m_cartridge[((m_currentRomBank + 1) * 0x4000) + (address - 0x4000)];
        }

        return m_cartridge[(m_currentRomBank * 0x4000) + (address - 0x4000)];
    }

    // Reading from RAM bank
    if (address >= 0xA000 && address <= 0xBFFF)
    {
        if (m_ramEnabled)
        {
            return m_ramBanks[(m_currentRamBank * 0x2000) + (address - 0xA000)];
        }
        else
        {
            return 0xFF;
        }
    }

    // Reading from Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    return m_memory[address];
}

void MBC1::write(size_t address, uint8_t value)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        m_ramEnabled = ((value & 0x0F) == 0x0A);
        return;
    }

    // ROM bank number
    if (address >= 0x2000 && address <= 0x3FFF)
    {
        uint8_t selectedRomBank = (value & 0x1F);
        m_currentRomBank = (m_currentRomBank & 0xE0) | selectedRomBank;
        return;
    }

    // RAM bank number or upper bits of ROM bank number
    if (address >= 0x4000 && address <= 0x5FFF)
    {
        m_currentRamBank = (value & 0x03);
        m_currentRomBank = (m_currentRomBank & 0x1F) | uint16_t(value & 0x03) << 5;
        return;
    }

    // Banking mode
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        m_currentBankingMode = (value & 1);
        return;
    }

    // Writing to RAM bank
    if (address >= 0xA000 && address <= 0xBFFF && m_ramEnabled)
    {
        m_ramBanks[(m_currentRamBank * 0x2000) + (address - 0xA000)] = value;

        m_ramBanksDirty = true;

        return;
    }

    // Writing to Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    //Enable/Disable APU
    if (address == 0xFF26)
    {
        bool newEnabled = (value >> 7) != 0;
        bool wasEnabled = (m_memory[0xFF26] >> 7) != 0;
        if (!newEnabled && wasEnabled)
        {
            for (uint16_t address = 0xFF10; address <= 0xFF25; address++)
            {
                m_memory[address] = 0;
            }
        }
        m_memory[0xFF26] = (m_memory[0xFF26] & 0x0F) | value;
        return;
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

void MBC1::saveRamBanksToFile(std::ofstream& file)
{
    file.write(reinterpret_cast<char*>(m_ramBanks), 0x8000);

    m_ramBanksDirty = false;
}

void MBC1::loadRamBanksFromFile(std::ifstream& file)
{
    file.read(reinterpret_cast<char*>(m_ramBanks), 0x8000);

    m_ramBanksDirty = false;
}

MBC2::MBC2(uint8_t* cartridge, size_t cartridgeSize)
    : Memory(cartridge, cartridgeSize)
{
}

MBC2::~MBC2()
{
}

uint8_t MBC2::read(size_t address)
{
    // Reading from ROM bank #
    if (address >= 0x4000 && address <= 0x7FFF)
    {
        return m_cartridge[(m_currentRomBank * 0x4000) + (address - 0x4000)];
    }

    // Reading from RAM bank
    if (address >= 0xA000 && address <= 0xBFFF)
    {
        return m_memory[0xA000 + (address & 0x1FF)];
    }

    // Reading from Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    return m_memory[address];
}

void MBC2::write(size_t address, uint8_t value)
{
    // RAM enable and ROM bank number
    if (address >= 0x0000 && address <= 0x3FFF)
    {
        if ((value & 0x0F) == 0x0A)
        {
            // TODO enable RAM
        }
        else
        {
            // TODO disable RAM
        }

        uint8_t selectedRomBank = (value & 0x0F);
        if (selectedRomBank == 0)
        {
            selectedRomBank++;
        }
        m_currentRomBank = (m_currentRomBank & 0xF0) | selectedRomBank;
        return;
    }

    // Writing to RAM bank
    if (address >= 0xA000 && address <= 0xBFFF)
    {
        m_memory[0xA000 + (address & 0x1FF)] = (value & 0x0F);

        m_ramBanksDirty = true;

        return;
    }

    // Writing to Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    //Enable/Disable APU
    if (address == 0xFF26)
    {
        bool newEnabled = (value >> 7) != 0;
        bool wasEnabled = (m_memory[0xFF26] >> 7) != 0;
        if (!newEnabled && wasEnabled)
        {
            for (uint16_t address = 0xFF10; address <= 0xFF25; address++)
            {
                m_memory[address] = 0;
            }
        }
        m_memory[0xFF26] = (m_memory[0xFF26] & 0x0F) | value;
        return;
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

    m_memory[address] = (value & 0x0F);
}

void MBC2::saveRamBanksToFile(std::ofstream& file)
{
    file.write(reinterpret_cast<char*>(&m_memory[0xA000]), 0x200);

    m_ramBanksDirty = false;
}

void MBC2::loadRamBanksFromFile(std::ifstream& file)
{
    file.read(reinterpret_cast<char*>(&m_memory[0xA000]), 0x200);

    m_ramBanksDirty = false;
}

MBC3::MBC3(uint8_t* cartridge, size_t cartridgeSize)
    : Memory(cartridge, cartridgeSize)
    , m_currentBankingMode(1)
{
}

MBC3::~MBC3()
{
}

uint8_t MBC3::read(size_t address)
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
        case 0x08:
        {
            return m_rtcSeconds;
            break;
        }
        case 0x09:
        {
            return m_rtcMinutes;
            break;
        }
        case 0x0A:
        {
            return m_rtcHours;
            break;
        }
        case 0x0B:
        {
            return m_rtcLowerDayCounter;
            break;
        }
        case 0x0C:
        {
            return m_rtcUpperDayCounter;
            break;
        }
        };
    }

    // Reading from Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    return m_memory[address];
}

void MBC3::write(size_t address, uint8_t value)
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

    // ROM bank number
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

    // RAM bank number
    if (address >= 0x4000 && address <= 0x5FFF)
    {
        m_currentRamBank = (value & 0x0F);
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
            m_ramBank0[(address - 0xA000)] = value;
            break;
        case 1:
            m_ramBank1[(address - 0xA000)] = value;
            break;
        case 2:
            m_ramBank2[(address - 0xA000)] = value;
            break;
        case 3:
            m_ramBank3[(address - 0xA000)] = value;
            break;
        case 0x08:
            m_rtcSeconds = value;
            break;
        case 0x09:
            m_rtcMinutes = value;
            break;
        case 0x0A:
            m_rtcHours = value;
            break;
        case 0x0B:
            m_rtcLowerDayCounter = value;
            break;
        case 0x0C:
            m_rtcUpperDayCounter = value;
            break;
        }

        if (m_currentRamBank >= 0 && m_currentRamBank <= 3)
        {
            m_ramBanksDirty = true;
        }

        return;
    }

    // Writing to Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    //Enable/Disable APU
    if (address == 0xFF26)
    {
        bool newEnabled = (value >> 7) != 0;
        bool wasEnabled = (m_memory[0xFF26] >> 7) != 0;
        if (!newEnabled && wasEnabled)
        {
            for (uint16_t address = 0xFF10; address <= 0xFF25; address++)
            {
                m_memory[address] = 0;
            }
        }
        m_memory[0xFF26] = (m_memory[0xFF26] & 0x0F) | value;
        return;
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

void MBC3::saveRamBanksToFile(std::ofstream& file)
{
    file.write(reinterpret_cast<char*>(m_ramBank0), 0x2000);
    file.write(reinterpret_cast<char*>(m_ramBank1), 0x2000);
    file.write(reinterpret_cast<char*>(m_ramBank2), 0x2000);
    file.write(reinterpret_cast<char*>(m_ramBank3), 0x2000);
    
    m_ramBanksDirty = false;
}

void MBC3::loadRamBanksFromFile(std::ifstream& file)
{
    file.read(reinterpret_cast<char*>(m_ramBank0), 0x2000);
    file.read(reinterpret_cast<char*>(m_ramBank1), 0x2000);
    file.read(reinterpret_cast<char*>(m_ramBank2), 0x2000);
    file.read(reinterpret_cast<char*>(m_ramBank3), 0x2000);

    m_ramBanksDirty = false;
}

MBC5::MBC5(uint8_t* cartridge, size_t cartridgeSize)
    : Memory(cartridge, cartridgeSize)
{
}

MBC5::~MBC5()
{
}

uint8_t MBC5::read(size_t address)
{
    // Reading from ROM bank
    if (address >= 0x4000 && address <= 0x7FFF)
    {
        return m_cartridge[(m_currentRomBank * 0x4000) + (address - 0x4000)];
    }

    // Reading from RAM bank
    if (address >= 0xA000 && address <= 0xBFFF)
    {
        return m_ramBanks[(m_currentRamBank * 0x2000) + (address - 0xA000)];
    }

    // Reading from Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    return m_memory[address];
}

void MBC5::write(size_t address, uint8_t value)
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

    // Least significant 8 bits of the ROM bank number
    if (address >= 0x2000 && address <= 0x2FFF)
    {
        uint8_t selectedRomBank = (value & 0xFF);
        m_currentRomBank = (m_currentRomBank & 0xFF00) | selectedRomBank;
        return;
    }

    // 9th bit of the ROM bank number
    if (address >= 0x3000 && address <= 0x3FFF)
    {
        uint16_t selectedRomBank = (value & 1) << 8;
        m_currentRomBank = (m_currentRomBank & 0x00FF) | selectedRomBank;
        return;
    }

    // RAM bank number
    if (address >= 0x4000 && address <= 0x5FFF)
    {
        m_currentRamBank = (value & 0x0F);
        return;
    }

    // Writing to RAM bank
    if (address >= 0xA000 && address <= 0xBFFF)
    {
        m_ramBanks[(m_currentRamBank * 0x2000) + (address - 0xA000)] = value;

        m_ramBanksDirty = true;

        return;
    }

    // Writing to Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    //Enable/Disable APU
    if (address == 0xFF26)
    {
        bool newEnabled = (value >> 7) != 0;
        bool wasEnabled = (m_memory[0xFF26] >> 7) != 0;
        if (!newEnabled && wasEnabled)
        {
            for (uint16_t address = 0xFF10; address <= 0xFF25; address++)
            {
                m_memory[address] = 0;
            }
        }
        m_memory[0xFF26] = (m_memory[0xFF26] & 0x0F) | value;
        return;
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

void MBC5::saveRamBanksToFile(std::ofstream& file)
{
    file.write(reinterpret_cast<char*>(m_ramBanks), 0x20000);

    m_ramBanksDirty = false;
}

void MBC5::loadRamBanksFromFile(std::ifstream& file)
{
    file.read(reinterpret_cast<char*>(m_ramBanks), 0x20000);

    m_ramBanksDirty = false;
}
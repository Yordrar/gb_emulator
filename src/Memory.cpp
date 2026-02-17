#include "Memory.h"

#include <Windows.h>

#include <algorithm>

#include "Emulator.h"
#include "CPU.h"

Memory::Memory(uint8_t* cartridge, size_t cartridgeSize)
    : m_cartridge(cartridge)
    , m_cartridgeSize(cartridgeSize)
{
    std::memcpy(m_memory, m_cartridge, std::min(0x7FFF, static_cast<int>(m_cartridgeSize)));

    if (!Emulator::isCGBMode())
    {
        for (uint16_t addr = 0xFF4C; addr <= 0xFF7F; ++addr)
        {
            m_memory[addr] = 0xFF; // CGB-only I/O Reg
        }
    }

    for (uint16_t i = 0; i < 32; ++i)
    {
        m_BGColorPaletteRam[i] = 0xFF; // CGB BG colors are initialized as white
    }
    m_OBJColorPaletteRam[0] = 0;
}

Memory::~Memory()
{
}

void Memory::updateRTC(uint64_t cyclesToEmulate)
{
    if (((m_rtcUpperDayCounter >> 6) & 1) == 1)
    {
        return;
    }

    m_rtcCounter += ((double)cyclesToEmulate / (double)CPU::s_frequencyHz);

    while (m_rtcCounter >= 1.0)
    {
        m_rtcCounter -= 1.0;
        m_rtcSeconds++;
        m_rtcSeconds &= 0x3F;
        if (m_rtcSeconds == 60)
        {
            m_rtcSeconds = 0;
            m_rtcMinutes++;
            m_rtcMinutes &= 0x3F;
            if (m_rtcMinutes == 60)
            {
                m_rtcMinutes = 0;
                m_rtcHours++;
                m_rtcHours &= 0x1F;
                if (m_rtcHours == 24)
                {
                    m_rtcHours = 0;
                    m_rtcLowerDayCounter++;
                    if (m_rtcLowerDayCounter == 0)
                    {
                        if ((m_rtcUpperDayCounter & 1) == 1)
                        {
                            m_rtcUpperDayCounter &= ~1;
                            m_rtcUpperDayCounter |= 0x80;
                        }
                        else
                        {
                            m_rtcUpperDayCounter |= 1;
                        }
                    }
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

    return handleCommonMemoryRead(address);
}

void Memory::write(size_t address, uint8_t value)
{
    handleCGBRegisterWrite(address, value);
    handleCommonMemoryWrite(address, value);

    m_memory[address] = value;
}

uint8_t Memory::readFromVramBank(size_t address, uint8_t bank)
{
    return m_vramBanks[(bank * 0x2000) + (address - 0x8000)];
}

void Memory::performHBlankDMATransfer()
{
    if (Emulator::isCGBMode() && m_hblankDMAInProgress && m_numBytesToCopyForDMATransfer > 0)
    {
        uint16_t numBytes = std::min((int)m_numBytesToCopyForDMATransfer, 0x10);
        for (uint16_t i = 0; i < numBytes; ++i)
        {
            write(m_hblankDMADestAddress + i, read(m_hblankDMASourceAddress + i));
        }
        m_hblankDMASourceAddress += numBytes;
        m_hblankDMADestAddress += numBytes;
        m_numBytesToCopyForDMATransfer -= numBytes;
    }
}

uint8_t Memory::handleCommonMemoryRead(size_t address)
{
    if (address >= 0x8000 && address <= 0x9FFF)
    {
        return m_vramBanks[(m_currentVramBank * 0x2000) + (address - 0x8000)];
    }

    if (address >= 0xD000 && address <= 0xDFFF)
    {
        return m_wramBanks[(m_currentWramBank * 0x1000) + (address - 0xD000)];
    }

    if (address >= 0xFF4C && address <= 0xFF7F && !Emulator::isCGBMode())
    {
        return 0xFF;
    }

    if (address == 0xFF4D)
    {
        return CPU::s_frequencyHz == CPU::s_CGBfrequencyHz ? 0x80 : 0;
    }

    if (address == 0xFF4F)
    {
        return 0xFE | (m_currentVramBank & 1);
    }

    if (address == 0xFF55)
    {
        uint8_t status = 0;
        status = (m_numBytesToCopyForDMATransfer / 16) - 1;
        if (m_numBytesToCopyForDMATransfer < 16) status = 0;
        status |= m_hblankDMAInProgress ? 0x00 : 0x80;
        if (m_numBytesToCopyForDMATransfer == 0) status = 0xFF;
        return status;
    }

    if (address == 0xFF69)
    {
        uint8_t addr = m_memory[0xFF68] & 0x3F;
        return ((uint8_t*)m_BGColorPaletteRam)[addr];
    }

    if (address == 0xFF6B)
    {
        uint8_t addr = m_memory[0xFF6A] & 0x3F;
        return ((uint8_t*)m_OBJColorPaletteRam)[addr];
    }

    return m_memory[address];
}

void Memory::handleCommonMemoryWrite(size_t address, uint8_t value)
{
    // Writing to VRAM
    if (address >= 0x8000 && address <= 0x9FFF)
    {
        m_vramBanks[(m_currentVramBank * 0x2000) + (address - 0x8000)] = value;
    }

    // Writing to WRAM bank
    if (address >= 0xD000 && address <= 0xDFFF)
    {
        m_wramBanks[(m_currentWramBank * 0x1000) + (address - 0xD000)] = value;
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
}

void Memory::handleCGBRegisterWrite(size_t address, uint8_t value)
{
    if (!Emulator::isCGBMode())
        return;

    if (address == 0xFF4D)
    {
        m_memory[address] = (value & 1);
    }

    if (address == 0xFF4F)
    {
        m_currentVramBank = (value & 1);
    }

    if (address == 0xFF51 || address == 0xFF52)
    {
        m_memory[address] = value;
        m_memory[0xFF52] &= 0xF0;
    }

    if (address == 0xFF53 || address == 0xFF54)
    {
        m_memory[address] = value;
        m_memory[0xFF54] &= 0xF0;
    }

    if (address == 0xFF55)
    {
        uint16_t sourceAddress = (((uint16_t)m_memory[0xFF51] << 8) | m_memory[0xFF52]);
        uint16_t destAddress = 0x8000 + (((uint16_t)m_memory[0xFF53] << 8) | m_memory[0xFF54]);
        uint16_t transferLen = (uint32_t(value & 0x7F) + 1) * 0x10;
        uint16_t transferMode = (value & 0x80) >> 7;

        if (!m_hblankDMAInProgress && transferMode == 0)
        {
            for (uint16_t i = 0; i < transferLen; ++i)
            {
                write(destAddress + i, read(sourceAddress + i));
            }
            m_numBytesToCopyForDMATransfer = 0;
        }
        else if(m_hblankDMAInProgress && transferMode == 0)
        {
            m_hblankDMAInProgress = false;
        }
        else
        {
            m_hblankDMAInProgress = true;
            m_hblankDMASourceAddress = sourceAddress;
            m_hblankDMADestAddress = destAddress;
            m_numBytesToCopyForDMATransfer = transferLen;
        }
    }

    if (address == 0xFF69)
    {
        uint8_t addr = m_memory[0xFF68] & 0x3F;
        ((uint8_t*)m_BGColorPaletteRam)[addr] = value;
        if (m_memory[0xFF68] & 0x80)
        {
            m_memory[0xFF68]++;
        }
    }

    if (address == 0xFF6B)
    {
        uint8_t addr = m_memory[0xFF6A] & 0x3F;
        ((uint8_t*)m_OBJColorPaletteRam)[addr] = value;
        m_OBJColorPaletteRam[0] = 0;
        if (m_memory[0xFF6A] & 0x80)
        {
            m_memory[0xFF6A]++;
        }
    }

    if (address == 0xFF70)
    {
        m_currentWramBank = (value & 0x7);
        if (m_currentWramBank == 0)
            m_currentWramBank++;
    }

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

    return handleCommonMemoryRead(address);
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

    handleCGBRegisterWrite(address, value);
    handleCommonMemoryWrite(address, value);

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

    return handleCommonMemoryRead(address);
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

    handleCGBRegisterWrite(address, value);
    handleCommonMemoryWrite(address, value);

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

    // Reading from RAM bank or RTC registers
    if (address >= 0xA000 && address <= 0xBFFF)
    {
        if (!m_enableRam)
        {
            return 0xFF;
        }

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
            return m_latchedRtcSeconds;
            break;
        }
        case 0x09:
        {
            return m_latchedRtcMinutes;
            break;
        }
        case 0x0A:
        {
            return m_latchedRtcHours;
            break;
        }
        case 0x0B:
        {
            return m_latchedRtcLowerDayCounter;
            break;
        }
        case 0x0C:
        {
            return m_latchedRtcUpperDayCounter;
            break;
        }
        };
    }

    // Reading from Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF)
    {
        address -= 0x2000;
    }

    return handleCommonMemoryRead(address);
}

void MBC3::write(size_t address, uint8_t value)
{
    // RAM and Timer enable
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        if ((value & 0x0F) == 0x0A)
        {
            m_enableRam = true;
        }
        else if ((value & 0x0F) == 0x00)
        {
            m_enableRam = false;
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

    // Latch RTC registers
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        if (value == 1 && m_latch == 0)
        {
            m_latchedRtcSeconds = m_rtcSeconds;
            m_latchedRtcMinutes = m_rtcMinutes;
            m_latchedRtcHours = m_rtcHours;
            m_latchedRtcLowerDayCounter = m_rtcLowerDayCounter;
            m_latchedRtcUpperDayCounter = m_rtcUpperDayCounter;
        }
        m_latch = value;
        return;
    }

    // Writing to RAM bank
    if (address >= 0xA000 && address <= 0xBFFF && m_enableRam)
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
            m_rtcSeconds = value & 0x3F;
            break;
        case 0x09:
            m_rtcMinutes = value & 0x3F;
            break;
        case 0x0A:
            m_rtcHours = value & 0x1F;
            break;
        case 0x0B:
            m_rtcLowerDayCounter = value;
            break;
        case 0x0C:
            m_rtcUpperDayCounter = value & 0xC1;
            break;
        }

        if (m_currentRamBank >= 0 && m_currentRamBank <= 3)
        {
            m_ramBanksDirty = true;
        }

        return;
    }

    handleCGBRegisterWrite(address, value);
    handleCommonMemoryWrite(address, value);

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

    return handleCommonMemoryRead(address);
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

    handleCGBRegisterWrite(address, value);
    handleCommonMemoryWrite(address, value);

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
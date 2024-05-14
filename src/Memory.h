#pragma once

#include <fstream>
#include <cstdint>

class Memory
{
public:
    Memory(uint8_t* cartridge, size_t cartridgeSize);
    ~Memory();

    void updateRTC(double deltaTimeSeconds);

    uint8_t& read(size_t address);
    void write(size_t address, uint8_t value);

    bool areRamBanksDirty() const { return m_ramBanksDirty; }
    void saveRamBanksToFile(std::ofstream& file);
    void loadRamBanksFromFile(std::ifstream& file);

private:
    uint8_t* m_cartridge;
    size_t m_cartridgeSize;

    /*
    -- Memory map of the Game Boy --
    |  Addresses  | Name |  Description
     0000h � 3FFFh  ROM0    Non - switchable ROM Bank
     4000h � 7FFFh  ROMX    Switchable ROM bank
     8000h � 9FFFh  VRAM    Video RAM, switchable(0 - 1) in GBC mode
     A000h � BFFFh  SRAM    External RAM in cartridge, often battery buffered
     C000h � CFFFh  WRAM0   Work RAM
     D000h � DFFFh  WRAMX   Work RAM, switchable(1 - 7) in GBC mode
     E000h � FDFFh  ECHO    Echo of WRAM0
     FE00h � FE9Fh  OAM     Sprite information table
     FEA0h � FEFFh  UNUSED  Unused
     FF00h � FF7Fh  I/O     I/O registers are mapped here
     FF80h � FFFEh  HRAM    Internal CPU RAM
     FFFFh          IE      Interrupt enable flags
    */
    uint8_t m_memory[0x10000] = {};

    uint8_t m_currentBankingMode = 0;

    uint8_t m_currentRomBank = 0;

    uint8_t m_currentRamBank = 0;
    uint8_t m_ramBank0[0x2000] = {};
    uint8_t m_ramBank1[0x2000] = {};
    uint8_t m_ramBank2[0x2000] = {};
    uint8_t m_ramBank3[0x2000] = {};
    bool m_ramBanksDirty = false;

    static const uint64_t RTCFrequencyHz = 32768 * 1024;
    double m_rtcCounter = 0;
    uint8_t m_rtcSeconds = 0;
    uint8_t m_rtcMinutes = 0;
    uint8_t m_rtcHours = 0;
    uint8_t m_rtcLowerDayCounter = 0;
    uint8_t m_rtcUpperDayCounter = 0;
};
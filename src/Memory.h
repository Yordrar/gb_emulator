#pragma once

#include <fstream>
#include <cstdint>

/*
-- Memory map of the Game Boy --
|  Addresses  | Name |  Description
 0000h – 3FFFh  ROM0    Non - switchable ROM Bank
 4000h – 7FFFh  ROMX    Switchable ROM bank
 8000h – 9FFFh  VRAM    Video RAM, switchable(0 - 1) in GBC mode
 A000h – BFFFh  SRAM    External RAM in cartridge, often battery buffered
 C000h – CFFFh  WRAM0   Work RAM
 D000h – DFFFh  WRAMX   Work RAM, switchable(1 - 7) in GBC mode
 E000h – FDFFh  ECHO    Echo of WRAM0
 FE00h – FE9Fh  OAM     Sprite information table
 FEA0h – FEFFh  UNUSED  Unused
 FF00h – FF7Fh  I/O     I/O registers are mapped here
 FF80h – FFFEh  HRAM    Internal CPU RAM
 FFFFh          IE      Interrupt enable flags
*/

class Memory
{
public:
    Memory(uint8_t* cartridge, size_t cartridgeSize);
    ~Memory();

    void updateRTC(double deltaTimeSeconds);

    virtual uint8_t read(size_t address);
    virtual void write(size_t address, uint8_t value);

    bool areRamBanksDirty() const { return m_ramBanksDirty; }
    virtual void saveRamBanksToFile(std::ofstream& file) {};
    virtual void loadRamBanksFromFile(std::ifstream& file) {};

protected:
    uint8_t* m_cartridge;
    size_t m_cartridgeSize;

    uint8_t m_memory[0x10000] = {};

    uint16_t m_currentRomBank = 1;

    bool m_ramBanksDirty = false;

    static const uint64_t RTCFrequencyHz = 32768 * 1024;
    double m_rtcCounter = 0;
    uint8_t m_rtcSeconds = 0;
    uint8_t m_rtcMinutes = 0;
    uint8_t m_rtcHours = 0;
    uint8_t m_rtcLowerDayCounter = 0;
    uint8_t m_rtcUpperDayCounter = 0;
};

class MBC1 : public Memory
{
public:
    MBC1(uint8_t* cartridge, size_t cartridgeSize);
    ~MBC1();

    virtual uint8_t read(size_t address) override;
    virtual void write(size_t address, uint8_t value) override;

    virtual void saveRamBanksToFile(std::ofstream& file) override;
    virtual void loadRamBanksFromFile(std::ifstream& file) override;

private:
    bool m_ramEnabled = false;

    uint8_t m_currentBankingMode = 0;

    uint16_t m_currentRomBank = 1;

    uint16_t m_currentRamBank = 0;
    uint8_t m_ramBanks[0x8000] = {};
};

class MBC2 : public Memory
{
public:
    MBC2(uint8_t* cartridge, size_t cartridgeSize);
    ~MBC2();

    virtual uint8_t read(size_t address) override;
    virtual void write(size_t address, uint8_t value) override;

    virtual void saveRamBanksToFile(std::ofstream& file) override;
    virtual void loadRamBanksFromFile(std::ifstream& file) override;

private:
    uint16_t m_currentRomBank = 1;
};

class MBC3 : public Memory
{
public:
    MBC3(uint8_t* cartridge, size_t cartridgeSize);
    ~MBC3();

    virtual uint8_t read(size_t address) override;
    virtual void write(size_t address, uint8_t value) override;

    virtual void saveRamBanksToFile(std::ofstream& file) override;
    virtual void loadRamBanksFromFile(std::ifstream& file) override;

private:
    uint8_t m_currentBankingMode = 0;

    uint8_t m_currentRomBank = 0;

    uint8_t m_currentRamBank = 0;
    uint8_t m_ramBank0[0x2000] = {};
    uint8_t m_ramBank1[0x2000] = {};
    uint8_t m_ramBank2[0x2000] = {};
    uint8_t m_ramBank3[0x2000] = {};
};

class MBC5 : public Memory
{
public:
    MBC5(uint8_t* cartridge, size_t cartridgeSize);
    ~MBC5();

    virtual uint8_t read(size_t address) override;
    virtual void write(size_t address, uint8_t value) override;

    virtual void saveRamBanksToFile(std::ofstream& file) override;
    virtual void loadRamBanksFromFile(std::ifstream& file) override;

private:
    uint16_t m_currentRomBank = 0;

    uint8_t m_currentRamBank = 0;
    uint8_t m_ramBanks[0x20000] = {};
};
#pragma once

#include <cstdint>

class CPU
{
public:
    CPU(uint8_t* cartridge);
    ~CPU();

private:
    static const uint64_t FrequencyHz = 4 * 1024 * 1024;

    uint8_t* m_cartridge;

    struct Registers
    {
        uint8_t A;
        uint8_t F;
        union
        {
            struct
            {
                uint8_t B;
                uint8_t C;
            };
            uint16_t BC;
        };
        union
        {
            struct
            {
                uint8_t D;
                uint8_t E;
            };
            uint16_t DE;
        };
        union
        {
            struct
            {
                uint8_t H;
                uint8_t L;
            };
            uint16_t HL;
        };
        uint16_t SP;
        uint16_t PC;
    } m_registers;

    /*
    -- Memory map of the Game Boy --
    |  Addresses  | Name |  Description 
     0000h – 3FFFh  ROM0    Non - switchable ROM Bank
     4000h – 7FFFh  ROMX    Switchable ROM bank
     8000h – 9FFFh  VRAM    Video RAM, switchable(0 - 1) in GBC mode
     A000h – BFFFh  SRAM    External RAM in cartridge, often battery buffered
     C000h – CFFFh  WRAM0   Work RAM
     D000h – DFFFh  WRAMX   Work RAM, switchable(1 - 7) in GBC mode
     E000h – FDFFh  ECHO    Description of the behaviour below
     FE00h – FE9Fh  OAM     Sprite information table
     FEA0h – FEFFh  UNUSED  Description of the behaviour below
     FF00h – FF7Fh  I/O     I/O registers are mapped here
     FF80h – FFFEh  HRAM    Internal CPU RAM
     FFFFh          IE      Interrupt enable flags
    */
    uint8_t m_memory[0x10000] = {};
};
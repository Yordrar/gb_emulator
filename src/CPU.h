#pragma once

#include <cstdint>

class CPU
{
public:
    CPU(uint8_t* cartridge, size_t cartridgeSize);
    ~CPU();

    enum Interrupt
    {
        VBlank = 0,
        LCD_STAT,
        Timer,
        Serial,
        Joypad,
    };

    static const uint64_t FrequencyHz = 4 * 1024 * 1024;

    uint64_t executeNextInstruction();
    void requestInterrupt(Interrupt interrupt);

private:
    void jumpToInterruptIfAnyPending();

    uint8_t getCarryFlagsFor8BitAddition(uint8_t op1, uint8_t op2);
    uint8_t getCarryFlagsFor8BitSubtraction(uint8_t op1, uint8_t op2);
    uint8_t getCarryFlagsFor16BitAddition(uint16_t op1, uint16_t op2);
    uint8_t getCarryFlagsFor16BitSubtraction(uint16_t op1, uint16_t op2);

    void rotateRegisterLeft(uint8_t& reg);
    void rotateRegisterLeftThroughCarry(uint8_t& reg);
    void rotateRegisterRight(uint8_t& reg);
    void rotateRegisterRightThroughCarry(uint8_t& reg);

    void shiftRegisterLeftArithmetically(uint8_t& reg);
    void shiftRegisterRightArithmetically(uint8_t& reg);
    void shiftRegisterRightLogically(uint8_t& reg);

    void testBitInRegister(uint8_t bit, uint8_t& reg);
    void setBitInRegister(uint8_t bit, uint8_t& reg);
    void resetBitInRegister(uint8_t bit, uint8_t& reg);

    uint8_t* m_cartridge;
    size_t m_cartridgeSize;

    struct Registers
    {
        union
        {
            struct
            {
                uint8_t A;
                uint8_t F;
            };
            uint16_t AF;
        };
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

    bool m_interruptMasterEnableFlag;

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
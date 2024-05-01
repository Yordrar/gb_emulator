#pragma once

#include <cstdint>

class Memory;

class CPU
{
public:
    CPU(Memory* memory);
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

    void update(double deltaTimeSeconds);
    void requestInterrupt(Interrupt interrupt);

private:
    uint64_t executeInstruction();
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

    struct Registers
    {
        union
        {
            struct
            {
                uint8_t F;
                uint8_t A;
            };
            uint16_t AF;
        };
        union
        {
            struct
            {
                uint8_t C;
                uint8_t B;
            };
            uint16_t BC;
        };
        union
        {
            struct
            {
                uint8_t E;
                uint8_t D;
            };
            uint16_t DE;
        };
        union
        {
            struct
            {
                uint8_t L;
                uint8_t H;
            };
            uint16_t HL;
        };
        uint16_t SP;
        uint16_t PC;
    } m_registers;

    bool m_interruptMasterEnableFlag;

    Memory* m_memory;
};
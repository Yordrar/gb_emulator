#pragma once

#include <cstdint>

class Memory;
class Joypad;

class CPU
{
public:
    CPU(Memory* memory, Joypad* joypad);
    ~CPU();

    enum Interrupt
    {
        VBlank = 0,
        LCD_STAT,
        Timer,
        Serial,
        JoyPad,
    };

    static const uint64_t FrequencyHz = 4 * 1024 * 1024;

    void requestInterrupt(Interrupt interrupt);
    uint64_t executeInstruction();

    bool isHalted() const { return m_isHalted; }
    bool hasWrittenToDIVLastCycle() const { return m_hasWrittenToDIVLastCycle; }

private:
    bool areTherePendingInterrupts();
    void jumpToPendingInterrupts();

    uint8_t getCarryFlagsFor8BitAddition(uint8_t op1, uint8_t op2);
    uint8_t getCarryFlagsFor8BitIncrement(uint8_t reg);
    uint8_t getCarryFlagsFor8BitSubtraction(uint8_t op1, uint8_t op2);
    uint8_t getCarryFlagsFor8BitDecrement(uint8_t reg);
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
    bool m_isHalted;
    bool m_hadPendingInterruptsWhenHalted;

    bool m_hasWrittenToDIVLastCycle = false;

    Memory* m_memory;

    Joypad* m_joypad;
};
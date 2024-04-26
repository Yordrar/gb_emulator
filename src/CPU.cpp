#include "CPU.h"

CPU::CPU(uint8_t* cartridge)
    : m_cartridge(cartridge)
{
    m_registers =
    {
        .A = 0x01,
        .F = 0xB0,
        .BC = 0x0013,
        .DE = 0x00D8,
        .HL = 0x014D,
        .SP = 0xFFFE,
    };
}

CPU::~CPU()
{
}

#include "CPU.h"

#include <cassert>
#include <cstring>
#include <cmath>

CPU::CPU(uint8_t* cartridge, size_t cartridgeSize)
    : m_cartridge(cartridge)
    , m_cartridgeSize(cartridgeSize)
{
    m_registers =
    {
        .AF = 0x01B0,
        .BC = 0x0013,
        .DE = 0x00D8,
        .HL = 0x014D,
        .SP = 0xFFFE,
    };
    m_registers.PC = 0x100;
    m_memory[0xFF05] = 0x00; // TIMA
    m_memory[0xFF06] = 0x00; // TMA
    m_memory[0xFF07] = 0x00; // TAC
    m_memory[0xFF10] = 0x80; // NR10
    m_memory[0xFF11] = 0xBF; // NR11
    m_memory[0xFF12] = 0xF3; // NR12
    m_memory[0xFF14] = 0xBF; // NR14
    m_memory[0xFF16] = 0x3F; // NR21
    m_memory[0xFF17] = 0x00; // NR22
    m_memory[0xFF19] = 0xBF; // NR24
    m_memory[0xFF1A] = 0x7F; // NR30
    m_memory[0xFF1B] = 0xFF; // NR31
    m_memory[0xFF1C] = 0x9F; // NR32
    m_memory[0xFF1E] = 0xBF; // NR33
    m_memory[0xFF20] = 0xFF; // NR41
    m_memory[0xFF21] = 0x00; // NR42
    m_memory[0xFF22] = 0x00; // NR43
    m_memory[0xFF23] = 0xBF; // NR30
    m_memory[0xFF24] = 0x77; // NR50
    m_memory[0xFF25] = 0xF3; // NR51
    m_memory[0xFF26] = 0xF1; // NR52
    m_memory[0xFF40] = 0x91; // LCDC
    m_memory[0xFF42] = 0x00; // SCY
    m_memory[0xFF43] = 0x00; // SCX
    m_memory[0xFF45] = 0x00; // LYC
    m_memory[0xFF47] = 0xFC; // BGP
    m_memory[0xFF48] = 0xFF; // OBP0
    m_memory[0xFF49] = 0xFF; // OBP1
    m_memory[0xFF4A] = 0x00; // WY
    m_memory[0xFF4B] = 0x00; // WX
    m_memory[0xFFFF] = 0x00; // IE

    std::memcpy(m_memory, m_cartridge, std::min(0x7FFF, static_cast<int>(m_cartridgeSize)));
}

CPU::~CPU()
{
}

uint64_t CPU::executeNextInstruction()
{
    uint8_t opcode = m_memory[m_registers.PC++];
    
    uint8_t n = 0;
    uint8_t carry = 0;

    switch (opcode)
    {
        // Misc opcodes
    case 0x00:
        return 4;
        break;

        // 8-bit load operations
    case 0x06:
        m_registers.B = m_memory[m_registers.PC++];
        return 8;
        break;
    case 0x0E:
        m_registers.C = m_memory[m_registers.PC++];
        return 8;
        break;
    case 0x16:
        m_registers.D = m_memory[m_registers.PC++];
        return 8;
        break;
    case 0x1E:
        m_registers.E = m_memory[m_registers.PC++];
        return 8;
        break;
    case 0x26:
        m_registers.H = m_memory[m_registers.PC++];
        return 8;
        break;
    case 0x2E:
        m_registers.L = m_memory[m_registers.PC++];
        return 8;
        break;
    case 0x7F:
        m_registers.A = m_registers.A;
        return 4;
        break;
    case 0x78:
        m_registers.A = m_registers.B;
        return 4;
        break;
    case 0x79:
        m_registers.A = m_registers.C;
        return 4;
        break;
    case 0x7A:
        m_registers.A = m_registers.D;
        return 4;
        break;
    case 0x7B:
        m_registers.A = m_registers.E;
        return 4;
        break;
    case 0x7C:
        m_registers.A = m_registers.H;
        return 4;
        break;
    case 0x7D:
        m_registers.A = m_registers.L;
        return 4;
        break;
    case 0x7E:
        m_registers.A = m_memory[m_registers.HL];
        return 8;
        break;
    case 0x40:
        m_registers.B = m_registers.B;
        return 4;
        break;
    case 0x41:
        m_registers.B = m_registers.C;
        return 4;
        break;
    case 0x42:
        m_registers.B = m_registers.D;
        return 4;
        break;
    case 0x43:
        m_registers.B = m_registers.E;
        return 4;
        break;
    case 0x44:
        m_registers.B = m_registers.H;
        return 4;
        break;
    case 0x45:
        m_registers.B = m_registers.L;
        return 4;
        break;
    case 0x46:
        m_registers.B = m_memory[m_registers.HL];
        return 8;
        break;
    case 0x48:
        m_registers.C = m_registers.B;
        return 4;
        break;
    case 0x49:
        m_registers.C = m_registers.C;
        return 4;
        break;
    case 0x4A:
        m_registers.C = m_registers.D;
        return 4;
        break;
    case 0x4B:
        m_registers.C = m_registers.E;
        return 4;
        break;
    case 0x4C:
        m_registers.C = m_registers.H;
        return 4;
        break;
    case 0x4D:
        m_registers.C = m_registers.L;
        return 4;
        break;
    case 0x4E:
        m_registers.C = m_memory[m_registers.HL];
        return 8;
        break;
    case 0x50:
        m_registers.D = m_registers.B;
        return 4;
        break;
    case 0x51:
        m_registers.D = m_registers.C;
        return 4;
        break;
    case 0x52:
        m_registers.D = m_registers.D;
        return 4;
        break;
    case 0x53:
        m_registers.D = m_registers.E;
        return 4;
        break;
    case 0x54:
        m_registers.D = m_registers.H;
        return 4;
        break;
    case 0x55:
        m_registers.D = m_registers.L;
        return 4;
        break;
    case 0x56:
        m_registers.D = m_memory[m_registers.HL];
        return 8;
        break;
    case 0x58:
        m_registers.E = m_registers.B;
        return 4;
        break;
    case 0x59:
        m_registers.E = m_registers.C;
        return 4;
        break;
    case 0x5A:
        m_registers.E = m_registers.D;
        return 4;
        break;
    case 0x5B:
        m_registers.E = m_registers.E;
        return 4;
        break;
    case 0x5C:
        m_registers.E = m_registers.H;
        return 4;
        break;
    case 0x5D:
        m_registers.E = m_registers.L;
        return 4;
        break;
    case 0x5E:
        m_registers.E = m_memory[m_registers.HL];
        return 8;
        break;
    case 0x60:
        m_registers.H = m_registers.B;
        return 4;
        break;
    case 0x61:
        m_registers.H = m_registers.C;
        return 4;
        break;
    case 0x62:
        m_registers.H = m_registers.D;
        return 4;
        break;
    case 0x63:
        m_registers.H = m_registers.E;
        return 4;
        break;
    case 0x64:
        m_registers.H = m_registers.H;
        return 4;
        break;
    case 0x65:
        m_registers.H = m_registers.L;
        return 4;
        break;
    case 0x66:
        m_registers.H = m_memory[m_registers.HL];
        return 8;
        break;
    case 0x68:
        m_registers.L = m_registers.B;
        return 4;
        break;
    case 0x69:
        m_registers.L = m_registers.C;
        return 4;
        break;
    case 0x6A:
        m_registers.L = m_registers.D;
        return 4;
        break;
    case 0x6B:
        m_registers.L = m_registers.E;
        return 4;
        break;
    case 0x6C:
        m_registers.L = m_registers.H;
        return 4;
        break;
    case 0x6D:
        m_registers.L = m_registers.L;
        return 4;
        break;
    case 0x6E:
        m_registers.L = m_memory[m_registers.HL];
        return 8;
        break;
    case 0x70:
        m_memory[m_registers.HL] = m_registers.B;
        return 8;
        break;
    case 0x71:
        m_memory[m_registers.HL] = m_registers.C;
        return 8;
        break;
    case 0x72:
        m_memory[m_registers.HL] = m_registers.D;
        return 8;
        break;
    case 0x73:
        m_memory[m_registers.HL] = m_registers.E;
        return 8;
        break;
    case 0x74:
        m_memory[m_registers.HL] = m_registers.H;
        return 8;
        break;
    case 0x75:
        m_memory[m_registers.HL] = m_registers.L;
        return 8;
        break;
    case 0x36:
        m_memory[m_registers.HL] = m_memory[m_registers.PC++];
        return 12;
        break;
    case 0x0A:
        m_registers.A = m_memory[m_registers.BC];
        return 8;
        break;
    case 0x1A:
        m_registers.A = m_memory[m_registers.DE];
        return 8;
        break;
    case 0xFA:
        m_registers.A = m_memory[(m_memory[m_registers.PC++] << 8) | (m_memory[m_registers.PC++])];
        return 16;
        break;
    case 0x3E:
        m_registers.A = m_memory[m_registers.PC++];
        return 8;
        break;
    case 0x47:
        m_registers.B = m_registers.A;
        return 4;
        break;
    case 0x4F:
        m_registers.C = m_registers.A;
        return 4;
        break;
    case 0x57:
        m_registers.D = m_registers.A;
        return 4;
        break;
    case 0x5F:
        m_registers.E = m_registers.A;
        return 4;
        break;
    case 0x67:
        m_registers.H = m_registers.A;
        return 4;
        break;
    case 0x6F:
        m_registers.L = m_registers.A;
        return 4;
        break;
    case 0x02:
        m_memory[m_registers.BC] = m_registers.A;
        return 8;
        break;
    case 0x12:
        m_memory[m_registers.DE] = m_registers.A;
        return 8;
        break;
    case 0x77:
        m_memory[m_registers.HL] = m_registers.A;
        return 8;
        break;
    case 0xEA:
        m_memory[(m_memory[m_registers.PC++] << 8) | (m_memory[m_registers.PC++])] = m_registers.A;
        return 16;
        break;
    case 0xF2:
        m_registers.A = m_memory[0xFF00 + m_registers.C];
        return 8;
        break;
    case 0xE2:
        m_memory[0xFF00 + m_registers.C] = m_registers.A;
        return 8;
        break;
    case 0x3A:
        m_registers.A = m_memory[m_registers.HL];
        m_registers.HL--;
        return 8;
        break;
    case 0x32:
        m_memory[m_registers.HL] = m_registers.A;
        m_registers.HL--;
        return 8;
        break;
    case 0x2A:
        m_registers.A = m_memory[m_registers.HL];
        m_registers.HL++;
        return 8;
        break;
    case 0x22:
        m_memory[m_registers.HL] = m_registers.A;
        m_registers.HL++;
        return 8;
        break;
    case 0xE0:
        m_memory[0xFF00 + m_memory[m_registers.PC++]] = m_registers.A;
        return 12;
        break;
    case 0xF0:
        m_registers.A = m_memory[0xFF00 + m_memory[m_registers.PC++]];
        return 12;
        break;

    // 16-bit load operations
    case 0x01:
        m_registers.BC = (m_memory[m_registers.PC++] << 8) | (m_memory[m_registers.PC++]);
        return 12;
        break;
    case 0x11:
        m_registers.DE = (m_memory[m_registers.PC++] << 8) | (m_memory[m_registers.PC++]);
        return 12;
        break;
    case 0x21:
        m_registers.HL = (m_memory[m_registers.PC++] << 8) | (m_memory[m_registers.PC++]);
        return 12;
        break;
    case 0x31:
        m_registers.SP = (m_memory[m_registers.PC++] << 8) | (m_memory[m_registers.PC++]);
        return 12;
        break;
    case 0xF9:
        m_registers.SP = m_registers.HL;
        return 8;
        break;
    case 0xF8:
        n = m_memory[m_registers.PC++];
        m_registers.F = getCarryFlagsForAddition(m_registers.SP, n) & 0b01111111;
        m_registers.HL = m_registers.SP + n;
        return 12;
        break;
    case 0x08:
        m_memory[(m_memory[m_registers.PC++] << 8) | (m_memory[m_registers.PC++])] = m_registers.SP;
        return 20;
        break;
    case 0xF5:
        m_memory[m_registers.SP] = m_registers.AF;
        m_registers.SP -= 2;
        return 16;
        break;
    case 0xC5:
        m_memory[m_registers.SP] = m_registers.BC;
        m_registers.SP -= 2;
        return 16;
        break;
    case 0xD5:
        m_memory[m_registers.SP] = m_registers.DE;
        m_registers.SP -= 2;
        return 16;
        break;
    case 0xE5:
        m_memory[m_registers.SP] = m_registers.HL;
        m_registers.SP -= 2;
        return 16;
        break;
    case 0xF1:
        m_registers.AF = m_memory[m_registers.SP];
        m_registers.SP += 2;
        return 12;
        break;
    case 0xC1:
        m_registers.BC = m_memory[m_registers.SP];
        m_registers.SP += 2;
        return 12;
        break;
    case 0xD1:
        m_registers.DE = m_memory[m_registers.SP];
        m_registers.SP += 2;
        return 12;
        break;
    case 0xE1:
        m_registers.HL = m_memory[m_registers.SP];
        m_registers.SP += 2;
        return 12;
        break;

    // 8-bit arithmetic operations
    case 0x87:
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.A);
        m_registers.A += m_registers.A;
        return 4;
        break;
    case 0x80:
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.B);
        m_registers.A += m_registers.B;
        return 4;
        break;
    case 0x81:
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.C);
        m_registers.A += m_registers.C;
        return 4;
        break;
    case 0x82:
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.D);
        m_registers.A += m_registers.D;
        return 4;
        break;
    case 0x83:
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.E);
        m_registers.A += m_registers.E;
        return 4;
        break;
    case 0x84:
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.H);
        m_registers.A += m_registers.H;
        return 4;
        break;
    case 0x85:
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.L);
        m_registers.A += m_registers.L;
        return 4;
        break;
    case 0x86:
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_memory[m_registers.HL]);
        m_registers.A += m_memory[m_registers.HL];
        return 8;
        break;
    case 0xC6:
        n = m_memory[m_registers.PC++];
        m_registers.F = getCarryFlagsForAddition(m_registers.A, n);
        m_registers.A += n;
        return 8;
        break;
    case 0x8F:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.A + carry);
        m_registers.A += m_registers.A + carry;
        return 4;
        break;
    case 0x88:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.B + carry);
        m_registers.A += m_registers.B + carry;
        return 4;
        break;
    case 0x89:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.C + carry);
        m_registers.A += m_registers.C + carry;
        return 4;
        break;
    case 0x8A:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.D + carry);
        m_registers.A += m_registers.D + carry;
        return 4;
        break;
    case 0x8B:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.E + carry);
        m_registers.A += m_registers.E + carry;
        return 4;
        break;
    case 0x8C:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.H + carry);
        m_registers.A += m_registers.H + carry;
        return 4;
        break;
    case 0x8D:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsForAddition(m_registers.A, m_registers.L + carry);
        m_registers.A += m_registers.L + carry;
        return 4;
        break;
    case 0x8E:
        carry = (m_registers.F & 0b00010000) >> 4;
        n = m_memory[m_registers.HL];
        m_registers.F = getCarryFlagsForAddition(m_registers.A, n + carry);
        m_registers.A += n + carry;
        return 8;
        break;
    case 0xCE:
        carry = (m_registers.F & 0b00010000) >> 4;
        n = m_memory[m_registers.PC++];
        m_registers.F = getCarryFlagsForAddition(m_registers.A, n + carry);
        m_registers.A += n + carry;
        return 8;
        break;
    case 0x97:
        m_registers.F = getCarryFlagsForSubtraction(m_registers.A, m_registers.A);
        m_registers.A -= m_registers.A;
        return 4;
        break;
    case 0x90:
        m_registers.F = getCarryFlagsForSubtraction(m_registers.A, m_registers.B);
        m_registers.A -= m_registers.B;
        return 4;
        break;
    case 0x91:
        m_registers.F = getCarryFlagsForSubtraction(m_registers.A, m_registers.C);
        m_registers.A -= m_registers.C;
        return 4;
        break;
    case 0x92:
        m_registers.F = getCarryFlagsForSubtraction(m_registers.A, m_registers.D);
        m_registers.A -= m_registers.D;
        return 4;
        break;
    case 0x93:
        m_registers.F = getCarryFlagsForSubtraction(m_registers.A, m_registers.E);
        m_registers.A -= m_registers.E;
        return 4;
        break;
    case 0x94:
        m_registers.F = getCarryFlagsForSubtraction(m_registers.A, m_registers.H);
        m_registers.A -= m_registers.H;
        return 4;
        break;
    case 0x95:
        m_registers.F = getCarryFlagsForSubtraction(m_registers.A, m_registers.L);
        m_registers.A -= m_registers.L;
        return 4;
        break;
    case 0x96:
        n = m_memory[m_registers.HL];
        m_registers.F = getCarryFlagsForSubtraction(m_registers.A, n);
        m_registers.A -= n;
        return 8;
        break;
    case 0xD6:
        n = m_memory[m_registers.PC++];
        m_registers.F = getCarryFlagsForSubtraction(m_registers.A, n);
        m_registers.A -= n;
        return 8;
        break;


        // Unimplemented or not supported opcode
    default:
        assert(false);
        break;
    }
}

uint8_t CPU::getCarryFlagsForAddition(uint16_t op1, uint16_t op2)
{
    uint8_t newFlag = 0;
    if (op1 + op2 > 0xFFFF)
    {
        newFlag |= 0b00010000;
    }
    if (((op1 & 0x000F) + (op2 & 0x000F)) > 0x000F)
    {
        newFlag |= 0b00100000;
    }
    if (((op1 + op2) & 0xFFFF) == 0)
    {
        newFlag |= 0b10000000;
    }
    return newFlag;
}

uint8_t CPU::getCarryFlagsForSubtraction(uint16_t op1, uint16_t op2)
{
    uint8_t newFlag = 0;
    if (op1 - op2 < 0xFFFF)
    {
        newFlag |= 0b00010000;
    }
    if (((op1 & 0x000F) - (op2 & 0x000F)) < 0x000F)
    {
        newFlag |= 0b00100000;
    }
    if (((op1 - op2) & 0xFFFF) == 0)
    {
        newFlag |= 0b10000000;
    }
    return newFlag;
}

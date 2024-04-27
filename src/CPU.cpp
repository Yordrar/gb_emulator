#include "CPU.h"

#include <Windows.h>
#include <string>
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
    
    uint16_t address = 0;
    uint8_t n = 0;
    uint8_t carry = 0;
    uint8_t newFlag = 0;

    switch (opcode)
    {
        // Misc operations
    case 0x00:
        return 4;
        break;
    case 0xCB:
        n = m_memory[m_registers.PC++];
        switch (n)
        {
        case 0x37:
            m_registers.A = ((m_registers.A & 0xF0) >> 4) | (m_registers.A & 0x0F << 4);
            m_registers.F = 0;
            if (m_registers.A == 0)
            {
                m_registers.F |= 0b10000000;
            }
            return 8;
            break;
        case 0x30:
            m_registers.B = ((m_registers.B & 0xF0) >> 4) | (m_registers.B & 0x0F << 4);
            m_registers.F = 0;
            if (m_registers.B == 0)
            {
                m_registers.F |= 0b10000000;
            }
            return 8;
            break;
        case 0x31:
            m_registers.C = ((m_registers.C & 0xF0) >> 4) | (m_registers.C & 0x0F << 4);
            m_registers.F = 0;
            if (m_registers.C == 0)
            {
                m_registers.F |= 0b10000000;
            }
            return 8;
            break;
        case 0x32:
            m_registers.D = ((m_registers.D & 0xF0) >> 4) | (m_registers.D & 0x0F << 4);
            m_registers.F = 0;
            if (m_registers.D == 0)
            {
                m_registers.F |= 0b10000000;
            }
            return 8;
            break;
        case 0x33:
            m_registers.E = ((m_registers.E & 0xF0) >> 4) | (m_registers.E & 0x0F << 4);
            m_registers.F = 0;
            if (m_registers.A == 0)
            {
                m_registers.F |= 0b10000000;
            }
            return 8;
            break;
        case 0x34:
            m_registers.H = ((m_registers.H & 0xF0) >> 4) | (m_registers.H & 0x0F << 4);
            m_registers.F = 0;
            if (m_registers.A == 0)
            {
                m_registers.F |= 0b10000000;
            }
            return 8;
            break;
        case 0x35:
            m_registers.L = ((m_registers.L & 0xF0) >> 4) | (m_registers.L & 0x0F << 4);
            m_registers.F = 0;
            if (m_registers.A == 0)
            {
                m_registers.F |= 0b10000000;
            }
            return 8;
            break;
        case 0x36:
            m_memory[m_registers.HL] = ((m_memory[m_registers.HL] & 0xF0) >> 4) | (m_memory[m_registers.HL] & 0x0F << 4);
            m_registers.F = 0;
            if (m_memory[m_registers.HL] == 0)
            {
                m_registers.F |= 0b10000000;
            }
            return 16;
            break;
        default:
            assert(false);
            return 0;
            break;
        }
        assert(false);
        return 0;
        break;
    case 0x27:
        // TODO: DAA opcode
        return 4;
        break;
    case 0x2F:
        m_registers.A = ~m_registers.A;
        m_registers.F |= 0b01100000;
        return 4;
        break;
    case 0x3F:
        n = m_registers.F & 0b00010000;
        n = ~n;
        m_registers.F &= 0b10011111;
        m_registers.F |= n;
        return 4;
        break;
    case 0x37:
        m_registers.F &= 0b10011111;
        m_registers.F |= 0b00010000;
        return 4;
        break;
    case 0x76:
        // TODO: HALT opcode
        return 4;
        break;
    case 0x10:
        // TODO: STOP opcode
        return 4;
        break;
    case 0xF3:
        // TODO: DI opcode
        return 4;
        break;
    case 0xFB:
        // TODO: EI opcode
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
        m_registers.A = m_memory[(m_memory[m_registers.PC++]) | (m_memory[m_registers.PC++] << 8)];
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
        m_memory[(m_memory[m_registers.PC++]) | (m_memory[m_registers.PC++] << 8)] = m_registers.A;
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
        m_registers.BC = (m_memory[m_registers.PC++]) | (m_memory[m_registers.PC++] << 8);
        return 12;
        break;
    case 0x11:
        m_registers.DE = (m_memory[m_registers.PC++]) | (m_memory[m_registers.PC++] << 8);
        return 12;
        break;
    case 0x21:
        m_registers.HL = (m_memory[m_registers.PC++]) | (m_memory[m_registers.PC++] << 8);
        return 12;
        break;
    case 0x31:
        m_registers.SP = (m_memory[m_registers.PC++]) | (m_memory[m_registers.PC++] << 8);
        return 12;
        break;
    case 0xF9:
        m_registers.SP = m_registers.HL;
        return 8;
        break;
    case 0xF8:
        n = m_memory[m_registers.PC++];
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.SP&0xFF, n) & 0b01111111;
        m_registers.HL = m_registers.SP + n;
        return 12;
        break;
    case 0x08:
        address = (m_memory[m_registers.PC++]) | (m_memory[m_registers.PC++] << 8);
        m_memory[address] = m_registers.SP & 0xFF;
        m_memory[address+1] = m_registers.SP >> 8;
        return 20;
        break;
    case 0xF5:
        m_memory[m_registers.SP--] = m_registers.A;
        m_memory[m_registers.SP--] = m_registers.F;
        return 16;
        break;
    case 0xC5:
        m_memory[m_registers.SP--] = m_registers.B;
        m_memory[m_registers.SP--] = m_registers.B;
        return 16;
        break;
    case 0xD5:
        m_memory[m_registers.SP--] = m_registers.D;
        m_memory[m_registers.SP--] = m_registers.E;
        return 16;
        break;
    case 0xE5:
        m_memory[m_registers.SP--] = m_registers.H;
        m_memory[m_registers.SP--] = m_registers.L;
        return 16;
        break;
    case 0xF1:
        m_registers.A = m_memory[m_registers.SP++];
        m_registers.F = m_memory[m_registers.SP++];
        return 12;
        break;
    case 0xC1:
        m_registers.B = m_memory[m_registers.SP++];
        m_registers.C = m_memory[m_registers.SP++];
        return 12;
        break;
    case 0xD1:
        m_registers.D = m_memory[m_registers.SP++];
        m_registers.E = m_memory[m_registers.SP++];
        return 12;
        break;
    case 0xE1:
        m_registers.H = m_memory[m_registers.SP++];
        m_registers.L = m_memory[m_registers.SP++];
        return 12;
        break;

    // 8-bit arithmetic operations
    case 0x87:
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.A);
        m_registers.A += m_registers.A;
        return 4;
        break;
    case 0x80:
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.B);
        m_registers.A += m_registers.B;
        return 4;
        break;
    case 0x81:
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.C);
        m_registers.A += m_registers.C;
        return 4;
        break;
    case 0x82:
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.D);
        m_registers.A += m_registers.D;
        return 4;
        break;
    case 0x83:
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.E);
        m_registers.A += m_registers.E;
        return 4;
        break;
    case 0x84:
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.H);
        m_registers.A += m_registers.H;
        return 4;
        break;
    case 0x85:
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.L);
        m_registers.A += m_registers.L;
        return 4;
        break;
    case 0x86:
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_memory[m_registers.HL]);
        m_registers.A += m_memory[m_registers.HL];
        return 8;
        break;
    case 0xC6:
        n = m_memory[m_registers.PC++];
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, n);
        m_registers.A += n;
        return 8;
        break;
    case 0x8F:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.A + carry);
        m_registers.A += m_registers.A + carry;
        return 4;
        break;
    case 0x88:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.B + carry);
        m_registers.A += m_registers.B + carry;
        return 4;
        break;
    case 0x89:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.C + carry);
        m_registers.A += m_registers.C + carry;
        return 4;
        break;
    case 0x8A:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.D + carry);
        m_registers.A += m_registers.D + carry;
        return 4;
        break;
    case 0x8B:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.E + carry);
        m_registers.A += m_registers.E + carry;
        return 4;
        break;
    case 0x8C:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.H + carry);
        m_registers.A += m_registers.H + carry;
        return 4;
        break;
    case 0x8D:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, m_registers.L + carry);
        m_registers.A += m_registers.L + carry;
        return 4;
        break;
    case 0x8E:
        carry = (m_registers.F & 0b00010000) >> 4;
        n = m_memory[m_registers.HL];
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, n + carry);
        m_registers.A += n + carry;
        return 8;
        break;
    case 0xCE:
        carry = (m_registers.F & 0b00010000) >> 4;
        n = m_memory[m_registers.PC++];
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.A, n + carry);
        m_registers.A += n + carry;
        return 8;
        break;
    case 0x97:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.A);
        m_registers.A -= m_registers.A;
        return 4;
        break;
    case 0x90:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.B);
        m_registers.A -= m_registers.B;
        return 4;
        break;
    case 0x91:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.C);
        m_registers.A -= m_registers.C;
        return 4;
        break;
    case 0x92:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.D);
        m_registers.A -= m_registers.D;
        return 4;
        break;
    case 0x93:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.E);
        m_registers.A -= m_registers.E;
        return 4;
        break;
    case 0x94:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.H);
        m_registers.A -= m_registers.H;
        return 4;
        break;
    case 0x95:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.L);
        m_registers.A -= m_registers.L;
        return 4;
        break;
    case 0x96:
        n = m_memory[m_registers.HL];
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, n);
        m_registers.A -= n;
        return 8;
        break;
    case 0xD6:
        n = m_memory[m_registers.PC++];
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, n);
        m_registers.A -= n;
        return 8;
        break;
    case 0x9F:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.A + carry);
        m_registers.A -= m_registers.A + carry;
        return 4;
        break;
    case 0x98:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.B + carry);
        m_registers.A -= m_registers.B + carry;
        return 4;
        break;
    case 0x99:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.C + carry);
        m_registers.A -= m_registers.C + carry;
        return 4;
        break;
    case 0x9A:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.D + carry);
        m_registers.A -= m_registers.D + carry;
        return 4;
        break;
    case 0x9B:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.E + carry);
        m_registers.A -= m_registers.E + carry;
        return 4;
        break;
    case 0x9C:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.H + carry);
        m_registers.A -= m_registers.H + carry;
        return 4;
        break;
    case 0x9D:
        carry = (m_registers.F & 0b00010000) >> 4;
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.L + carry);
        m_registers.A -= m_registers.L + carry;
        return 4;
        break;
    case 0x9E:
        carry = (m_registers.F & 0b00010000) >> 4;
        n = m_memory[m_registers.HL];
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, n + carry);
        m_registers.A -= n + carry;
        return 8;
        break;
    case 0xDE:
        carry = (m_registers.F & 0b00010000) >> 4;
        n = m_memory[m_registers.PC++];
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, n + carry);
        m_registers.A -= n + carry;
        return 8;
        break;
    case 0xA7:
        n = m_registers.A & m_registers.A;
        newFlag |= 0b00100000;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xA0:
        n = m_registers.A & m_registers.B;
        newFlag |= 0b00100000;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xA1:
        n = m_registers.A & m_registers.C;
        newFlag |= 0b00100000;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xA2:
        n = m_registers.A & m_registers.D;
        newFlag |= 0b00100000;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xA3:
        n = m_registers.A & m_registers.E;
        newFlag |= 0b00100000;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xA4:
        n = m_registers.A & m_registers.H;
        newFlag |= 0b00100000;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xA5:
        n = m_registers.A & m_registers.L;
        newFlag |= 0b00100000;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xA6:
        n = m_registers.A & m_memory[m_registers.HL];
        newFlag |= 0b00100000;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 8;
        break;
    case 0xE6:
        n = m_registers.A & m_memory[m_registers.PC++];
        newFlag |= 0b00100000;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 8;
        break;
    case 0xB7:
        n = m_registers.A | m_registers.A;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xB0:
        n = m_registers.A | m_registers.B;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xB1:
        n = m_registers.A | m_registers.C;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xB2:
        n = m_registers.A | m_registers.D;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xB3:
        n = m_registers.A | m_registers.E;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xB4:
        n = m_registers.A | m_registers.H;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xB5:
        n = m_registers.A | m_registers.L;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xB6:
        n = m_registers.A | m_memory[m_registers.HL];
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 8;
        break;
    case 0xF6:
        n = m_registers.A | m_memory[m_registers.PC++];
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 8;
        break;
    case 0xAF:
        n = m_registers.A ^ m_registers.A;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xA8:
        n = m_registers.A ^ m_registers.B;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xA9:
        n = m_registers.A ^ m_registers.C;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xAA:
        n = m_registers.A ^ m_registers.D;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xAB:
        n = m_registers.A ^ m_registers.E;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xAC:
        n = m_registers.A ^ m_registers.H;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xAD:
        n = m_registers.A ^ m_registers.L;
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 4;
        break;
    case 0xAE:
        n = m_registers.A ^ m_memory[m_registers.HL];
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 8;
        break;
    case 0xEE:
        n = m_registers.A ^ m_memory[m_registers.PC++];
        if (n == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        m_registers.A = n;
        return 8;
        break;
    case 0xBF:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.A);
        return 4;
        break;
    case 0xB8:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.B);
        return 4;
        break;
    case 0xB9:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.C);
        return 4;
        break;
    case 0xBA:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.D);
        return 4;
        break;
    case 0xBB:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.E);
        return 4;
        break;
    case 0xBC:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.H);
        return 4;
        break;
    case 0xBD:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_registers.L);
        return 4;
        break;
    case 0xBE:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_memory[m_registers.HL]);
        return 8;
        break;
    case 0xFE:
        m_registers.F = getCarryFlagsFor8BitSubtraction(m_registers.A, m_memory[m_registers.PC++]);
        return 8;
        break;
    case 0x3C:
        if (m_registers.A + 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F &= 0b10111111;
        if ((m_registers.A & 0x0F) + 1 > 0x0F)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.A++;
        return 4;
        break;
    case 0x04:
        if (m_registers.B + 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F &= 0b10111111;
        if ((m_registers.B & 0x0F) + 1 > 0x0F)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.B++;
        return 4;
        break;
    case 0x0C:
        if (m_registers.C + 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F &= 0b10111111;
        if ((m_registers.C & 0x0F) + 1 > 0x0F)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.C++;
        return 4;
        break;
    case 0x14:
        if (m_registers.D + 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F &= 0b10111111;
        if ((m_registers.D & 0x0F) + 1 > 0x0F)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.D++;
        return 4;
        break;
    case 0x1C:
        if (m_registers.E + 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F &= 0b10111111;
        if ((m_registers.E & 0x0F) + 1 > 0x0F)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.E++;
        return 4;
        break;
    case 0x24:
        if (m_registers.H + 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F &= 0b10111111;
        if ((m_registers.H & 0x0F) + 1 > 0x0F)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.H++;
        return 4;
        break;
    case 0x2C:
        if (m_registers.L + 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F &= 0b10111111;
        if ((m_registers.L & 0x0F) + 1 > 0x0F)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.L++;
        return 4;
        break;
    case 0x34:
        if (m_memory[m_registers.HL] + 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F &= 0b10111111;
        if ((m_memory[m_registers.HL] & 0x0F) + 1 > 0x0F)
        {
            m_registers.F |= 0b00100000;
        }
        m_memory[m_registers.HL]++;
        return 12;
        break;
    case 0x3D:
        if (m_registers.A - 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F |= 0b01000000;
        if ((m_registers.A & 0xF) == 0)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.A--;
        return 4;
        break;
    case 0x05:
        if (m_registers.B - 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F |= 0b01000000;
        if ((m_registers.B & 0xF) == 0)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.B--;
        return 4;
        break;
    case 0x0D:
        if (m_registers.C - 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F |= 0b01000000;
        if ((m_registers.C & 0xF) == 0)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.C--;
        return 4;
        break;
    case 0x15:
        if (m_registers.D - 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F |= 0b01000000;
        if ((m_registers.D & 0xF) == 0)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.D--;
        return 4;
        break;
    case 0x1D:
        if (m_registers.E - 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F |= 0b01000000;
        if ((m_registers.E & 0xF) == 0)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.E--;
        return 4;
        break;
    case 0x25:
        if (m_registers.H - 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F |= 0b01000000;
        if ((m_registers.H & 0xF) == 0)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.H--;
        return 4;
        break;
    case 0x2D:
        if (m_registers.L - 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F |= 0b01000000;
        if ((m_registers.L & 0xF) == 0)
        {
            m_registers.F |= 0b00100000;
        }
        m_registers.L--;
        return 4;
        break;
    case 0x35:
        if (m_memory[m_registers.HL] - 1 == 0)
        {
            m_registers.F |= 0b10000000;
        }
        m_registers.F |= 0b01000000;
        if ((m_memory[m_registers.HL] & 0xF) == 0)
        {
            m_registers.F |= 0b00100000;
        }
        m_memory[m_registers.HL]--;
        return 12;
        break;

        // 16-bit arithmetic operations
    case 0x09:
        m_registers.F = getCarryFlagsFor16BitAddition(m_registers.HL, m_registers.BC);
        m_registers.HL += m_registers.BC;
        return 8;
        break;
    case 0x19:
        m_registers.F = getCarryFlagsFor16BitAddition(m_registers.HL, m_registers.DE);
        m_registers.HL += m_registers.DE;
        return 8;
        break;
    case 0x29:
        m_registers.F = getCarryFlagsFor16BitAddition(m_registers.HL, m_registers.HL);
        m_registers.HL += m_registers.HL;
        return 8;
        break;
    case 0x39:
        m_registers.F = getCarryFlagsFor16BitAddition(m_registers.HL, m_registers.SP);
        m_registers.HL += m_registers.SP;
        return 8;
        break;
    case 0xE8:
        n = m_memory[m_registers.PC++];
        m_registers.F = getCarryFlagsFor8BitAddition(m_registers.SP&0xFF, n)&0b00111111;
        m_registers.SP += n;
        return 16;
        break;
    case 0x03:
        m_registers.BC++;
        return 8;
        break;
    case 0x13:
        m_registers.DE++;
        return 8;
        break;
    case 0x23:
        m_registers.HL++;
        return 8;
        break;
    case 0x33:
        m_registers.SP++;
        return 8;
        break;
    case 0x0B:
        m_registers.BC--;
        return 8;
        break;
    case 0x1B:
        m_registers.DE--;
        return 8;
        break;
    case 0x2B:
        m_registers.HL--;
        return 8;
        break;
    case 0x3B:
        m_registers.SP--;
        return 8;
        break;

        // Rotate and shift operations
    case 0x07:
        n = (m_registers.A & 0b10000000)>>7;
        m_registers.A <<= 1;
        m_registers.A |= n;
        newFlag |= (n << 4);
        if (m_registers.A == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        return 4;
        break;
    case 0x17:
        n = (m_registers.A & 0b10000000) >> 7;
        m_registers.A <<= 1;
        m_registers.A |= ((m_registers.F&0b00010000) >> 4);
        newFlag |= (n << 4);
        if (m_registers.A == 0)
        {
            newFlag |= 0b10000000;
        }
        m_registers.F = newFlag;
        return 4;
        break;

        // Unimplemented or not supported opcode
    default:
        OutputDebugStringA("Unknown opcode: ");
        OutputDebugStringA(std::to_string(opcode).c_str());
        OutputDebugStringA("\n");
        assert(false);
        return 0;
        break;
    }
}

uint8_t CPU::getCarryFlagsFor8BitAddition(uint8_t op1, uint8_t op2)
{
    uint8_t newFlag = 0;
    if (op1 + op2 > 0xFF)
    {
        newFlag |= 0b00010000;
    }
    if (((op1 & 0x0F) + (op2 & 0x0F)) > 0x0F)
    {
        newFlag |= 0b00100000;
    }
    if (((op1 + op2) & 0xFF) == 0)
    {
        newFlag |= 0b10000000;
    }
    return newFlag;
}

uint8_t CPU::getCarryFlagsFor8BitSubtraction(uint8_t op1, uint8_t op2)
{
    uint8_t newFlag = 0;
    if (op2 > op1)
    {
        newFlag |= 0b00010000;
    }
    if ((op2 & 0x0F) > (op1 & 0x0F))
    {
        newFlag |= 0b00100000;
    }
    if (op1 == op2)
    {
        newFlag |= 0b10000000;
    }
    newFlag |= 0b01000000;
    return newFlag;
}

uint8_t CPU::getCarryFlagsFor16BitAddition(uint16_t op1, uint16_t op2)
{
    uint8_t newFlag = 0;
    if (op1 + op2 > 0xFFFF)
    {
        newFlag |= 0b00010000;
    }
    if (((op1 & 0x0F00) + (op2 & 0x0F00)) > 0x0F00)
    {
        newFlag |= 0b00100000;
    }
    newFlag |= (m_registers.F&0b10111111);
    return newFlag;
}

uint8_t CPU::getCarryFlagsFor16BitSubtraction(uint16_t op1, uint16_t op2)
{
    uint8_t newFlag = 0;
    if (op2 > op1)
    {
        newFlag |= 0b00010000;
    }
    if ((op2 & 0x0F00) > (op1 & 0x0F00))
    {
        newFlag |= 0b00100000;
    }
    if (op1 == op2)
    {
        newFlag |= 0b10000000;
    }
    newFlag |= 0b01000000;
    return newFlag;
}

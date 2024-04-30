#include "Memory.h"

#include <string>

Memory::Memory(uint8_t* cartridge, size_t cartridgeSize)
    : m_cartridge(cartridge)
    , m_cartridgeSize(cartridgeSize)
{
    std::memcpy(m_memory, m_cartridge, std::min(0x7FFF, static_cast<int>(m_cartridgeSize)));
}

Memory::~Memory()
{
}

uint8_t& Memory::read(size_t address)
{
    return m_memory[address];
}

void Memory::write(size_t address, uint8_t value)
{
    m_memory[address] = value;
}
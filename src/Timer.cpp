#include "Timer.h"

#include "CPU.h"
#include "Memory.h"

Timer::Timer(CPU* cpu, Memory* memory)
    : m_cpu(cpu)
    , m_memory(memory)
    , m_dividerRegister(memory->read(0xFF04))
    , m_timerCounter(memory->read(0xFF05))
{
}

Timer::~Timer()
{
}

void Timer::update(uint64_t cyclesToEmulate)
{
    uint8_t timerStop = (m_memory->read(0xFF07) & 0x4) >> 2;
    if (timerStop == 0)
    {
        return;
    }

    double deltaTimeSeconds = static_cast<double>(cyclesToEmulate) / CPU::FrequencyHz;

    // DIV
    if (m_memory->read(0xFF04) != static_cast<uint8_t>(m_dividerRegister))
    {
        m_dividerRegister = 0;
        m_memory->write(0xFF04, 0);
    }
    m_dividerRegister += 16384 * deltaTimeSeconds;
    m_dividerRegister = static_cast<double>(static_cast<uint32_t>(m_dividerRegister) % 256);
    m_memory->write(0xFF04, static_cast<uint8_t>(m_dividerRegister));

    // TIMA
    uint8_t inputClockSelect = (m_memory->read(0xFF07) & 0x3);
    uint64_t clockFrequency = clockFrequenciesHz[inputClockSelect];
    double clockTicksToIncrement = clockFrequency * deltaTimeSeconds;
    m_timerCounter += clockTicksToIncrement;
    if (m_timerCounter > 255.0)
    {
        m_timerCounter = m_memory->read(0xFF06);
        m_memory->write(0xFF05, m_memory->read(0xFF06));
        m_cpu->requestInterrupt(CPU::Interrupt::Timer);
    }
    else
    {
        m_memory->write(0xFF05, static_cast<uint8_t>(m_timerCounter));
    }
}

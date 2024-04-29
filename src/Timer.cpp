#include "Timer.h"

#include "CPU.h"

Timer::Timer(CPU* cpu)
    : m_cpu(cpu)
    , m_dividerRegister(cpu->getMemory()[0xFF04])
    , m_timerCounter(cpu->getMemory()[0xFF05])
{
}

Timer::~Timer()
{
}

void Timer::update(double deltaTimeSeconds)
{
    uint8_t* memory = m_cpu->getMemory();

    uint8_t timerStop = (memory[0xFF07] & 0x4) >> 2;
    if (timerStop == 0)
    {
        return;
    }

    // DIV
    if (memory[0xFF04] != static_cast<uint8_t>(m_dividerRegister) % 256)
    {
        m_dividerRegister = 0;
        memory[0xFF04] = 0;
    }
    else
    {
        m_dividerRegister += 16384 * deltaTimeSeconds;
        memory[0xFF04] = static_cast<uint8_t>(m_dividerRegister) % 256;
    }

    // TIMA
    uint8_t inputClockSelect = (memory[0xFF07] & 0x3);
    uint64_t clockFrequency = clockFrequenciesHz[inputClockSelect];
    double clockTicksToIncrement = clockFrequency * deltaTimeSeconds;
    m_timerCounter += clockTicksToIncrement;
    if (m_timerCounter > 255.0)
    {
        m_timerCounter = memory[0xFF06];
        memory[0xFF05] = memory[0xFF06];
        m_cpu->requestInterrupt(CPU::Interrupt::Timer);
    }
    else
    {
        memory[0xFF05] = static_cast<uint8_t>(m_timerCounter);
    }
}

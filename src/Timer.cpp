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
    // For some reason some games need the timer but they don't enable it, so ignore TAC. Possible bug?
    /*uint8_t timerStop = (m_memory->read(0xFF07) & 0x4) >> 2;
    if (timerStop == 0)
    {
        return;
    }*/

    for (int i = 0; i < cyclesToEmulate; i++)
    {
        m_timerClock++;
        // DIV
        if (m_memory->read(0xFF04) != static_cast<uint8_t>(m_dividerRegister))
        {
            m_dividerRegister = 0;
            m_memory->write(0xFF04, 0);
        }
        if ((m_timerClock % 256) == 0)
        {
            m_dividerRegister = (m_dividerRegister + 1) % 256;
        }
        m_memory->write(0xFF04, static_cast<uint8_t>(m_dividerRegister));

        // TIMA
        uint8_t inputClockSelect = (m_memory->read(0xFF07) & 0x3);
        uint64_t clockFrequency = clockFrequenciesHz[inputClockSelect];
        if (m_timerClock % (CPU::FrequencyHz / clockFrequency) == 0)
        {
            m_timerCounter++;
        }
        if (m_timerCounter > 255)
        {
            m_timerCounter = m_memory->read(0xFF06);
            m_cpu->requestInterrupt(CPU::Interrupt::Timer);
        }
        m_memory->write(0xFF05, static_cast<uint8_t>(m_timerCounter));
    }
}

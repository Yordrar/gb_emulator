#include "Timer.h"

#include <Windows.h>
#include <cassert>

#include "CPU.h"
#include "Memory.h"

Timer::Timer(CPU* cpu, Memory* memory)
    : m_cpu(cpu)
    , m_memory(memory)
    , m_tac(m_memory->read(0xFF07))
    , m_div(memory->read(0xFF04))
    , m_tima(memory->read(0xFF05))
{
}

Timer::~Timer()
{
}

void Timer::update(uint64_t cyclesToEmulate)
{
    uint8_t tac = m_memory->read(0xFF07);
    if (m_tac != tac)
    {
        m_tac = tac;
        checkFallingEdge();
    }
    uint8_t timerEnable = (m_tac & 0x4) >> 2;

    uint8_t tima = m_memory->read(0xFF05);
    if (m_tima != tima)
    {
        m_tima = tima;
        m_timaOverflowed = false;
    }

    if (m_timaOverflowed)
    {
        m_timaOverflowed = false;
        m_tima = m_memory->read(0xFF06);
        m_cpu->requestInterrupt(CPU::Interrupt::Timer);
    }

    if (m_cpu->hasWrittenToDIVLastCycle())
    {
        m_timerClock = 0;
        m_div = 0;
        m_memory->write(0xFF04, 0);
        checkFallingEdge();
    }

    for (int i = 0; i < cyclesToEmulate; i++)
    {
        m_timerClock++;

        // DIV
        if (m_cpu->isHalted())
        {
            m_div = 0;
        }
        else if ((m_timerClock % (CPU::FrequencyHz / 16384)) == 0)
        {
            m_div++;
        }
        m_memory->write(0xFF04, m_div);

        if (timerEnable == 1)
        {
            // TIMA
            uint8_t inputClockSelect = (m_tac & 0x3);
            uint64_t clockFrequency = clockFrequenciesHz[inputClockSelect];
            if (m_timerClock % static_cast<uint16_t>(CPU::FrequencyHz / clockFrequency) == 0)
            {
                m_tima++;
            }
            if (m_tima > 255)
            {
                m_timaOverflowed = true;
                m_tima = 0;
            }
            m_memory->write(0xFF05, static_cast<uint8_t>(m_tima));
        }

        uint8_t bit = 0;
        uint8_t inputClockSelect = (m_memory->read(0xFF07) & 0x3);
        switch (inputClockSelect)
        {
        case 0:
            bit = 9;
            break;
        case 1:
            bit = 3;
            break;
        case 2:
            bit = 5;
            break;
        case 3:
            bit = 7;
            break;
        default:
            assert(false);
            break;
        }

        m_lastFallingEdgeCheckAndResult = (uint8_t((m_timerClock >> bit) & 0x01)) & ((m_tac >> 2) & 0x01);
    }
}

void Timer::checkFallingEdge()
{
    uint8_t bit = 0;
    uint8_t inputClockSelect = (m_memory->read(0xFF07) & 0x3);
    switch (inputClockSelect)
    {
    case 0:
        bit = 9;
        break;
    case 1:
        bit = 3;
        break;
    case 2:
        bit = 5;
        break;
    case 3:
        bit = 7;
        break;
    default:
        assert(false);
        break;
    }

    uint8_t andResult = (uint8_t((m_timerClock >> bit) & 0x01)) & ((m_tac >> 2) & 0x01);

    if ((m_lastFallingEdgeCheckAndResult & ~andResult) != 0) {
        m_tima++;

        if (m_tima > 255)
        {
            m_timaOverflowed = true;
            m_tima = 0;
        }

        m_memory->write(0xFF05, static_cast<uint8_t>(m_tima));
    }

    m_lastFallingEdgeCheckAndResult = andResult;
}

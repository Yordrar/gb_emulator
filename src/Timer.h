#pragma once

#include <cstdint>

class CPU;
class Memory;

class Timer
{
public:
    Timer(CPU* cpu, Memory* memory);
    ~Timer();

    void update(uint64_t cyclesToEmulate);

private:
    void checkFallingEdge();

    uint64_t clockFrequenciesHz[4] = { 4096, 262144, 65536, 16384 };

    CPU* m_cpu;
    Memory* m_memory;

    uint8_t m_tac = 0;

    uint16_t m_timerClock = 0;
    uint8_t m_div = 0;
    uint32_t m_tima = 0;
    bool m_timaOverflowed = false;
    uint8_t m_lastFallingEdgeCheckAndResult = 0;
};
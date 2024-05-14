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
    uint64_t clockFrequenciesHz[4] = { 4096, 262144, 65536, 16384 };

    CPU* m_cpu;
    Memory* m_memory;

    uint64_t m_timerClock = 0;
    uint8_t m_dividerRegister;
    uint32_t m_timerCounter;
};
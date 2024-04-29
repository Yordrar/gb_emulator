#pragma once

#include <cstdint>

class CPU;

class Timer
{
public:
    Timer(CPU* cpu);
    ~Timer();

    void update(double deltaTimeSeconds);

private:
    uint64_t clockFrequenciesHz[4] = { 4096, 262144, 65536, 16384 };

    CPU* m_cpu;

    double m_dividerRegister;
    double m_timerCounter;
};
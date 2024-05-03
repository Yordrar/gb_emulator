#pragma once

#include <cstdint>

class CPU;
class Memory;

class Joypad
{
public:
    Joypad(CPU* cpu, Memory* memory);
    ~Joypad();

    void update(double deltaTimeSeconds);

private:
    CPU* m_cpu;
    Memory* m_memory;
};
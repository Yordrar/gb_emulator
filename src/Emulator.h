#pragma once

#include <resource/ResourceHandle.h>

#include <string>
#include <cstdint>
#include <memory>

class CPU;
class Timer;
class LCD;
class Memory;
class Joypad;
class Sound;

class Emulator
{
public:
    Emulator(ResourceHandle frameTexture, uint8_t* frameTextureData);
    ~Emulator();

    struct CartridgeInfo
    {
        std::string m_name;
        uint8_t m_type;
    };

    void openRomFile(char const* romFilename);
    CartridgeInfo getCartridgeInfo() const { return m_cartridgeInfo; }

    void emulate();
    void processInput(WPARAM wParam, LPARAM lParam);

    void saveBatteryBackedRamToFile();
    void loadSavFileToRam();

private:
    ResourceHandle m_frameTexture;
    uint8_t* m_frameTextureData;

    std::string m_romFilename;
    std::unique_ptr<uint8_t[]> m_cartridge;
    size_t m_cartridgeSize;
    CartridgeInfo m_cartridgeInfo;

    std::unique_ptr<Memory> m_memory;
    std::unique_ptr<CPU> m_cpu;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<LCD> m_lcd;
    std::unique_ptr<Joypad> m_joypad;
    std::unique_ptr<Sound> m_sound;
};
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
        std::string m_name = "";
        bool m_isColorGB = false;
        bool m_isNonCGBCompatible = false;
        bool m_hasSGBFunctions = false;
        uint8_t m_type = 0;
        uint64_t m_romSize = 0;
        uint64_t m_numRomBanks = 0;
        uint64_t m_ramSize = 0;
        uint64_t m_numRamBanks = 0;

        std::string getCartridgeTypeStr() const;
        bool hasBatteryBackedRam() const;
        bool hasMBC1() const;
        bool hasMBC2() const;
        bool hasMBC3() const;
        bool hasMBC5() const;
    };

    void openRomFile(char const* romFilename);
    void closeCurrentRom() { m_hasOpenedRomFile = false; }
    bool hasOpenedRomFile() const { return m_hasOpenedRomFile; }
    CartridgeInfo getCartridgeInfo() const { return m_cartridgeInfo; }

    void emulate();
    void processKeyboardInput(WPARAM wParam, LPARAM lParam);

    void saveBatteryBackedRamToFile();
    void loadSavFileToRam();

private:
    void extractCartridgeInfo();

    bool m_hasOpenedRomFile = false;

    ResourceHandle m_frameTexture;
    uint8_t* m_frameTextureData;

    std::string m_romFilename;
    std::unique_ptr<uint8_t[]> m_cartridge;
    size_t m_cartridgeSize = 0;
    CartridgeInfo m_cartridgeInfo;

    std::unique_ptr<Memory> m_memory;
    std::unique_ptr<CPU> m_cpu;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<LCD> m_lcd;
    std::unique_ptr<Joypad> m_joypad;
    std::unique_ptr<Sound> m_sound;
};
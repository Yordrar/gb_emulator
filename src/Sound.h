#pragma once

#include <SDL.h>
#include <SDL_audio.h>

#include <cstdint>

class Memory;

class Sound
{
public:
    Sound(Memory* memory);
    ~Sound();

    void update(uint64_t cyclesToEmulate);

    static const uint64_t sc_SampleRate = 48000;

private:
    void updateChannel1Data();
    void updateChannel2Data();
    void updateChannel3Data();
    void updateChannel4Data();
    void updateFrequencyTimer(int64_t& frequencyTimer, uint64_t newFrequency, uint8_t* dutyPosition);
    void updateFrequencyTimerChannel3();
    void updateFrequencyTimerChannel4();
    void handleEnvelopeClock(int64_t& periodTimer, uint8_t& envelopeSweep, uint8_t& currentVolume, uint8_t& envelopeDirection);
    void handleSweepClock();
    void handleLengthClock(uint8_t& channelEnabled, uint8_t& lengthEnabled, uint8_t& lengthCounter);
    void handleLengthClock(uint8_t& channelEnabled, uint8_t& lengthEnabled, uint16_t& lengthCounter);
    uint64_t calculateCh1NewFrequencyAndOverflowCheck();

    Memory* m_memory;

    SDL_AudioDeviceID m_audioDevice;
    SDL_AudioSpec m_audioSpec;

    static const uint64_t sc_AudioDataBufferSize = 1024;
    float m_audioDataBuffer[sc_AudioDataBufferSize] = { 0 };
    uint32_t m_audioDataBufferSampleCount = 0;

    uint64_t m_frameSequencer = 0;
    uint64_t m_sampleClock = 0;
    uint8_t m_waveDutyTable[4][8] = {
        0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,1,1,1,
        0,1,1,1,1,1,1,0,
    };

    // Channel 1 data
    uint8_t m_ch1Enabled = 0;

    uint8_t m_ch1SweepEnabled = 0;
    uint8_t m_ch1SweepTimer = 0;
    uint8_t m_ch1SweepTime = 0;
    uint8_t m_ch1SweepDecrease = 0;
    uint8_t m_ch1SweepShift = 0;
    uint64_t m_ch1ShadowFrequency = 0;

    uint8_t m_ch1LengthTimer = 0;
    uint8_t m_ch1LengthEnabled = 0;
    uint8_t m_ch1SoundLength = 0;

    uint8_t m_ch1WavePatternDuty = 0;
    uint8_t m_ch1DutyPosition = 0;

    uint64_t m_ch1Frequency = 0;
    int64_t m_ch1FrequencyTimer = 0;

    uint8_t m_ch1EnvelopeInitial = 0;
    uint8_t m_ch1EnvelopeDirection = 0;
    uint8_t m_ch1EnvelopePeriod = 0;
    int64_t m_ch1PeriodTimer = 0;
    uint8_t m_ch1CurrentVolume = 0;

    // Channel 2 data
    uint8_t m_ch2Enabled = 0;
    uint8_t m_ch2LengthTimer = 0;
    uint8_t m_ch2LengthEnabled = 0;
    uint8_t m_ch2SoundLength = 0;

    uint8_t m_ch2WavePatternDuty = 0;
    uint8_t m_ch2DutyPosition = 0;
    uint64_t m_ch2Frequency = 0;
    int64_t m_ch2FrequencyTimer = 0;


    uint8_t m_ch2EnvelopeInitial = 0;
    uint8_t m_ch2EnvelopeDirection = 0;
    uint8_t m_ch2EnvelopeSweep = 0;
    int64_t m_ch2PeriodTimer = 0;
    uint8_t m_ch2CurrentVolume = 0;

    // Channel 3 data
    uint8_t m_ch3Enabled = 0;
    uint16_t m_ch3LengthTimer = 0;
    uint8_t m_ch3LengthEnabled = 0;
    uint16_t m_ch3SoundLength = 0;
    uint8_t m_ch3WavePosition = 0;
    uint64_t m_ch3Frequency = 0;
    int64_t m_ch3FrequencyTimer = 0;
    uint8_t m_ch3OutputLevel = 0;

    // Channel 4 data
    uint8_t m_ch4Enabled = 0;

    uint64_t m_ch4Frequency = 0;
    int64_t m_ch4FrequencyTimer = 0;

    uint8_t m_ch4LengthTimer = 0;
    uint8_t m_ch4LengthEnabled = 0;
    uint8_t m_ch4SoundLength = 0;

    uint8_t m_ch4EnvelopeInitial = 0;
    uint8_t m_ch4EnvelopeDirection = 0;
    uint8_t m_ch4EnvelopeSweep = 0;
    int64_t m_ch4PeriodTimer = 0;
    uint8_t m_ch4CurrentVolume = 0;

    uint8_t m_ch4shiftClockFrequency = 0;
    uint8_t m_ch4counterStep = 0;
    uint8_t m_ch4divRatioFrequencies = 0;

    uint16_t m_LFSR = 0;
};
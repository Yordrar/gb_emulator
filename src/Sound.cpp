#include "Sound.h"

#include <Windows.h>

#include <cassert>

#include "Memory.h"
#include "CPU.h"

Sound::Sound(Memory* memory)
    : m_memory(memory)
{
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    SDL_AudioSpec wantSpec =
    {
        .freq = sc_SampleRate,
        .format = AUDIO_F32,
        .channels = 2,
        .samples = sc_AudioDataBufferSize / 2,
        .callback = nullptr,
    };
    m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wantSpec, &m_audioSpec, 0);
    SDL_PauseAudioDevice(m_audioDevice, 0);
}

Sound::~Sound()
{
    SDL_PauseAudioDevice(m_audioDevice, 1);
    SDL_CloseAudioDevice(m_audioDevice);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void Sound::update(uint64_t cyclesToEmulate)
{
    uint8_t NR50 = m_memory->read(0xFF24);
    uint8_t NR51 = m_memory->read(0xFF25);
    uint8_t NR52 = m_memory->read(0xFF26);
    uint8_t leftVolume = ((NR50 & 0x70) >> 4);
    uint8_t rightVolume = (NR50 & 0x07);
    uint8_t outputCh4ToLeft = (NR51 & 0x80) >> 7;
    uint8_t outputCh3ToLeft = (NR51 & 0x40) >> 6;
    uint8_t outputCh2ToLeft = (NR51 & 0x20) >> 5;
    uint8_t outputCh1ToLeft = (NR51 & 0x10) >> 4;
    uint8_t outputCh4ToRight = (NR51 & 0x08) >> 3;
    uint8_t outputCh3ToRight = (NR51 & 0x04) >> 2;
    uint8_t outputCh2ToRight = (NR51 & 0x02) >> 1;
    uint8_t outputCh1ToRight = (NR51 & 0x01);
    uint8_t soundEnable = (NR52 & 0x80);

    updateChannel1Data();
    updateChannel2Data();
    updateChannel3Data();
    updateChannel4Data();

    for (uint64_t i = 0; i < cyclesToEmulate; i++)
    {
        m_sampleClock++;

        if ((m_sampleClock % 8192) == 0)
        {
            m_sampleClock = 0;

            m_frameSequencer = (m_frameSequencer + 1) & 7;

            if ((m_frameSequencer & 1) == 0)
            {
                handleLengthClock(m_ch1Enabled, m_ch1LengthEnabled, m_ch1LengthTimer);
                handleLengthClock(m_ch2Enabled, m_ch2LengthEnabled, m_ch2LengthTimer);
                handleLengthClock(m_ch3Enabled, m_ch3LengthEnabled, m_ch3LengthTimer);
                handleLengthClock(m_ch4Enabled, m_ch4LengthEnabled, m_ch4LengthTimer);
            }
            if (m_frameSequencer == 2 || m_frameSequencer == 6)
            {
                // Only channel 1 has sweep
                handleSweepClock();
            }
            if (m_frameSequencer == 7)
            {
                handleEnvelopeClock(m_ch1PeriodTimer, m_ch1EnvelopePeriod, m_ch1CurrentVolume, m_ch1EnvelopeDirection);
                handleEnvelopeClock(m_ch2PeriodTimer, m_ch2EnvelopeSweep, m_ch2CurrentVolume, m_ch2EnvelopeDirection);
                handleEnvelopeClock(m_ch4PeriodTimer, m_ch4EnvelopeSweep, m_ch4CurrentVolume, m_ch4EnvelopeDirection);
            }
        }

        updateFrequencyTimer(m_ch1FrequencyTimer, ((2048 - m_ch1Frequency) * 4), &m_ch1DutyPosition);
        updateFrequencyTimer(m_ch2FrequencyTimer, ((2048 - m_ch2Frequency) * 4), &m_ch2DutyPosition);
        updateFrequencyTimerChannel3();
        updateFrequencyTimerChannel4();

        if ((m_sampleClock % (CPU::s_frequencyHz / Sound::sc_SampleRate)) == 0)
        {
            if (soundEnable)
            {
                uint8_t waveSampleCh1 = m_waveDutyTable[m_ch1WavePatternDuty][m_ch1DutyPosition] * m_ch1CurrentVolume;
                float sampleCh1 = (waveSampleCh1 / 7.5f) - 1.0f;

                uint8_t waveSampleCh2 = m_waveDutyTable[m_ch2WavePatternDuty][m_ch2DutyPosition] * m_ch2CurrentVolume;
                float sampleCh2 = (waveSampleCh2 / 7.5f) - 1.0f;

                uint8_t waveSampleCh3 = ( m_memory->read(0xFF30 + (m_ch3WavePosition / 2) ) >> ((m_ch3WavePosition & 1) != 0 ? 0 : 4) ) & 0x0F;
                waveSampleCh3 = waveSampleCh3 >> m_ch3OutputLevel;
                float sampleCh3 = (waveSampleCh3 / 7.5f) - 1.0f;

                float sampleCh4 = static_cast<float>( ((~m_LFSR) & 1) * m_ch4CurrentVolume );
                sampleCh4 = (sampleCh4 / 7.5f) - 1.0f;

                float leftSample =
                        ((sampleCh1 * outputCh1ToLeft * m_ch1Enabled) +
                        (sampleCh2 * outputCh2ToLeft * m_ch2Enabled) +
                        (sampleCh3 * outputCh3ToLeft * m_ch3Enabled) +
                        (sampleCh4 * outputCh4ToRight * m_ch4Enabled) ) / 4.0f;
                float rightSample =
                        ((sampleCh1 * outputCh1ToRight * m_ch1Enabled) +
                        (sampleCh2 * outputCh2ToRight * m_ch2Enabled) +
                        (sampleCh3 * outputCh3ToRight * m_ch3Enabled) +
                        (sampleCh4 * outputCh4ToRight * m_ch4Enabled) ) / 4.0f;
                m_audioDataBuffer[m_audioDataBufferSampleCount++] = leftSample * (leftVolume / 30.0f);
                m_audioDataBuffer[m_audioDataBufferSampleCount++] = rightSample * (rightVolume / 30.0f);
            }
            else
            {
                m_audioDataBuffer[m_audioDataBufferSampleCount++] = 0.0f;
                m_audioDataBuffer[m_audioDataBufferSampleCount++] = 0.0f;
            }
            if (m_audioDataBufferSampleCount >= sc_AudioDataBufferSize)
            {
                while (SDL_GetQueuedAudioSize(m_audioDevice) > (sc_AudioDataBufferSize * sizeof(float)))
                {
                    Sleep(1);
                }
                SDL_QueueAudio(m_audioDevice, m_audioDataBuffer, (sc_AudioDataBufferSize * sizeof(float)));
                m_audioDataBufferSampleCount = 0;
            }
        }
    }

    // Update NR52
    m_memory->write(0xFF26, (NR52 & 0x80) | (m_ch4Enabled<<3) | (m_ch3Enabled<<2) | (m_ch2Enabled<<1) | (m_ch1Enabled));
}

void Sound::updateChannel1Data()
{
    uint8_t NR10 = m_memory->read(0xFF10);
    uint8_t NR11 = m_memory->read(0xFF11);
    uint8_t NR12 = m_memory->read(0xFF12);
    uint8_t NR13 = m_memory->read(0xFF13);
    uint8_t NR14 = m_memory->read(0xFF14);

    m_ch1SweepTime = (NR10 >> 4) & 0x7;
    m_ch1SweepDecrease = (NR10 >> 3) & 0x1;
    m_ch1SweepShift = NR10 & 0x7;
    m_ch1WavePatternDuty = (NR11 >> 6) & 0x3;
    m_ch1EnvelopeInitial = (NR12 & 0xF0) >> 4;
    m_ch1EnvelopeDirection = (NR12 & 0x08) >> 3;
    m_ch1DACEnabled = (NR12 & 0xF8) != 0 ? 1 : 0;
    if (m_ch1DACEnabled == 0)
    {
        m_ch1Enabled = 0;
    }
    m_ch1EnvelopePeriod = (NR12 & 0x07);
    m_ch1Frequency = (uint64_t(NR13) >> 0) | (uint64_t(NR14 & 7) << 8);
    m_ch1LengthEnabled = (NR14 >> 6) & 1;

    if ((NR14 & 0x80) != 0 && m_ch1DACEnabled == 1)
    {
        // Trigger event
        m_ch1Enabled = 1;
        if (m_ch1LengthTimer == 0)
        {
            m_ch1LengthTimer = 64;
        }
        m_ch1FrequencyTimer = ((2048 - m_ch1Frequency) * 4);
        m_ch1PeriodTimer = m_ch1EnvelopePeriod;
        m_ch1CurrentVolume = m_ch1EnvelopeInitial;
        m_ch1ShadowFrequency = m_ch1Frequency;
        m_ch1SweepTimer = m_ch1SweepTime == 0 ? 8 : m_ch1SweepTime;
        m_ch1SweepEnabled = (m_ch1SweepTime != 0) || (m_ch1SweepShift != 0) ? 1 : 0;
        if (m_ch1SweepShift > 0)
        {
            calculateCh1NewFrequencyAndOverflowCheck();
        }
    }
    if ((NR11 & 0b00111111) != 0 && m_ch1LengthEnabled)
    {
        m_ch1LengthTimer = 64 - (NR11 & 0b00111111);
        m_memory->write(0xFF16, NR11 & 0b11000000);
    }
    m_memory->write(0xFF14, NR14 & 0x7F);
}

void Sound::updateChannel2Data()
{
    uint8_t NR21 = m_memory->read(0xFF16);
    uint8_t NR22 = m_memory->read(0xFF17);
    uint8_t NR23 = m_memory->read(0xFF18);
    uint8_t NR24 = m_memory->read(0xFF19);

    m_ch2WavePatternDuty = (NR21 >> 6) & 0x3;
    m_ch2EnvelopeInitial = (NR22 & 0xF0) >> 4;
    m_ch2EnvelopeDirection = (NR22 & 0x08) >> 3;
    m_ch2DACEnabled = (NR22 & 0xF8) != 0 ? 1 : 0;
    if (m_ch2DACEnabled == 0)
    {
        m_ch2Enabled = 0;
    }
    m_ch2EnvelopeSweep = (NR22 & 0x07);
    m_ch2Frequency = (uint64_t(NR23) >> 0) | (uint64_t(NR24 & 7) << 8);
    m_ch2LengthEnabled = (NR24 & 0x4) >> 6;

    if ((NR24 & 0x80) != 0 && m_ch2DACEnabled == 1)
    {
        // Trigger event
        m_ch2Enabled = 1;
        if (m_ch2LengthTimer == 0)
        {
            m_ch2LengthTimer = 64;
        }
        m_ch2FrequencyTimer = ((2048 - m_ch2Frequency) * 4);
        m_ch2PeriodTimer = m_ch2EnvelopeSweep;
        m_ch2CurrentVolume = m_ch2EnvelopeInitial;
    }
    if ((NR21 & 0b00111111) != 0 && m_ch2LengthEnabled)
    {
        m_ch2LengthTimer = 64 - (NR21 & 0b00111111);
        m_memory->write(0xFF16, NR21 & 0b11000000);
    }
    m_memory->write(0xFF19, NR24 & 0x7F);
}

void Sound::updateChannel3Data()
{
    uint8_t NR30 = m_memory->read(0xFF1A);
    uint8_t NR31 = m_memory->read(0xFF1B);
    uint8_t NR32 = m_memory->read(0xFF1C);
    uint8_t NR33 = m_memory->read(0xFF1D);
    uint8_t NR34 = m_memory->read(0xFF1E);

    m_ch3DACEnabled = (NR30 & 0x80) >> 7;
    if (m_ch3DACEnabled == 0)
    {
        m_ch3Enabled = 0;
    }

    switch ((NR32 >> 5) & 0x3)
    {
    case 0:
        m_ch3OutputLevel = 4;
        break;
    case 1:
        m_ch3OutputLevel = 0;
        break;
    case 2:
        m_ch3OutputLevel = 1;
        break;
    case 3:
        m_ch3OutputLevel = 2;
        break;
    default:
        assert(false);
        break;
    }
    m_ch3Frequency = (uint64_t(NR33) >> 0) | (uint64_t(NR34 & 7) << 8);

    m_ch3LengthEnabled = (NR34 & 0x4) >> 6;

    if ((NR34 & 0x80) != 0 && m_ch3DACEnabled == 1)
    {
        // Trigger event
        m_ch3Enabled = 1;
        if (m_ch3LengthTimer == 0)
        {
            m_ch3LengthTimer = 256;
        }
        m_ch3FrequencyTimer = ((2048 - m_ch3Frequency) * 2);
        m_ch3WavePosition = 0;
    }
    if (NR31 != 0 && (NR34 & 0x40) != 0)
    {
        m_ch3LengthTimer = 256 - NR31;
        m_memory->write(0xFF1B, 0);
    }
    m_memory->write(0xFF1E, NR34 & 0x7F);
}

void Sound::updateChannel4Data()
{
    uint8_t NR41 = m_memory->read(0xFF20);
    uint8_t NR42 = m_memory->read(0xFF21);
    uint8_t NR43 = m_memory->read(0xFF22);
    uint8_t NR44 = m_memory->read(0xFF23);

    m_ch4LengthEnabled = (NR44 & 0x40) != 0 ? 1 : 0;

    if ((NR41 & 0b00111111) != 0 && m_ch4LengthEnabled != 0)
    {
        m_ch4LengthTimer = 64 - (NR41 & 0b00111111);
        m_memory->write(0xFF20, NR41 & 0b11000000);
    }

    m_ch4EnvelopeInitial = (NR42 & 0xF0) >> 4;
    m_ch4EnvelopeDirection = (NR42 & 0x08) >> 3;
    m_ch4DACEnabled = (NR42 & 0xF8) != 0 ? 1 : 0;
    if (m_ch4DACEnabled == 0)
    {
        m_ch4Enabled = 0;
    }
    m_ch4EnvelopeSweep = (NR42 & 0x07);

    m_ch4shiftClockFrequency = (NR43 & 0xF0) >> 4;
    m_ch4counterStep = (NR43 & 0x08) >> 3;
    m_ch4divRatioFrequencies = NR43 & 0x07;

    if ((NR44 & 0x80) != 0 && m_ch4DACEnabled == 1)
    {
        // Trigger event
        m_ch4Enabled = 1;
        if (m_ch4LengthTimer == 0)
        {
            m_ch4LengthTimer = 64;
        }
        m_ch4FrequencyTimer = int64_t(m_ch4divRatioFrequencies > 0 ? m_ch4divRatioFrequencies * 16 : 8) << m_ch4shiftClockFrequency;
        m_ch4PeriodTimer = m_ch4EnvelopeSweep;
        m_ch4CurrentVolume = m_ch4EnvelopeInitial;
        m_LFSR = 0x7FFF;
    }
    m_memory->write(0xFF23, NR44 & 0x7F);
}

void Sound::updateFrequencyTimer(int64_t& frequencyTimer, uint64_t newFrequency, uint8_t* dutyPosition)
{
    frequencyTimer--;
    if (frequencyTimer <= 0)
    {
        frequencyTimer = newFrequency;
        if (dutyPosition)
        {
            *dutyPosition = (*dutyPosition + 1) % 8;
        }
    }
}

void Sound::updateFrequencyTimerChannel3()
{
    m_ch3FrequencyTimer--;
    if (m_ch3FrequencyTimer <= 0)
    {
        m_ch3FrequencyTimer = ((2048 - m_ch3Frequency) * 2);
        m_ch3WavePosition = (m_ch3WavePosition + 1) % 32;
    }
}

void Sound::updateFrequencyTimerChannel4()
{
    m_ch4FrequencyTimer--;
    if (m_ch4FrequencyTimer <= 0)
    {
        m_ch4FrequencyTimer = int64_t(m_ch4divRatioFrequencies > 0 ? m_ch4divRatioFrequencies * 16 : 8) << m_ch4shiftClockFrequency;

        uint16_t xorResult = ((m_LFSR & 1) >> 0) ^ ((m_LFSR & 2) >> 1);
        m_LFSR = ((m_LFSR >> 1) & 0x3FFF) | (xorResult << 14);
        if (m_ch4counterStep == 1)
        {
            m_LFSR = (m_LFSR & ~(1 << 6)) | (xorResult << 6);
        }
    }
}

void Sound::handleEnvelopeClock(int64_t& periodTimer, uint8_t& envelopeSweep, uint8_t& currentVolume, uint8_t& envelopeDirection)
{
    if (envelopeSweep == 0) return;

    if (periodTimer > 0)
    {
        periodTimer--;
    }
    if (periodTimer == 0)
    {
        periodTimer = envelopeSweep;
        if ((currentVolume < 0xF && envelopeDirection) || (currentVolume > 0x0 && !envelopeDirection)) {
            if (envelopeDirection)
            {
                currentVolume++;
            }
            else
            {
                currentVolume--;
            }
        }
    }
}

void Sound::handleSweepClock()
{
    if (m_ch1SweepTimer > 0)
    {
        m_ch1SweepTimer--;
    }

    if (m_ch1SweepTimer == 0)
    {
        m_ch1SweepTimer = m_ch1SweepTime == 0 ? 8 : m_ch1SweepTime;
        if (m_ch1SweepEnabled && m_ch1SweepTime != 0)
        {
            uint64_t newFrequency = calculateCh1NewFrequencyAndOverflowCheck();

            if (newFrequency <= 2047 && m_ch1SweepShift > 0)
            {
                m_ch1Frequency = newFrequency;
                m_ch1ShadowFrequency = newFrequency;

                // Write back the new frequency to NR13 and NR14
                m_memory->write(0xFF13, m_ch1Frequency & 0xFF);
                m_memory->write(0xFF14, ((m_ch1Frequency & 0x700) >> 8) | (m_ch1LengthEnabled << 6));

                calculateCh1NewFrequencyAndOverflowCheck();
            }
        }
    }
}

void Sound::handleLengthClock(uint8_t& channelEnabled, uint8_t& lengthEnabled, uint8_t& lengthCounter)
{
    if (lengthEnabled != 0 && lengthCounter > 0) {
        lengthCounter--;
        if (lengthCounter == 0) {
            channelEnabled = 0;
        }
    }
}

void Sound::handleLengthClock(uint8_t& channelEnabled, uint8_t& lengthEnabled, uint16_t& lengthCounter)
{
    if (lengthEnabled != 0 && lengthCounter > 0) {
        lengthCounter--;
        if (lengthCounter == 0) {
            channelEnabled = 0;
        }
    }
}

uint64_t Sound::calculateCh1NewFrequencyAndOverflowCheck()
{
    uint64_t newFrequency = m_ch1ShadowFrequency >> m_ch1SweepShift;

    if (m_ch1SweepDecrease)
    {
        newFrequency = m_ch1ShadowFrequency - newFrequency;
    }
    else
    {
        newFrequency = m_ch1ShadowFrequency + newFrequency;
    }

    if (newFrequency > 2047)
    {
        m_ch1Enabled = 0;
    }

    return newFrequency;
}

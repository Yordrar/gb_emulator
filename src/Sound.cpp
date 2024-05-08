#include "Sound.h"

#include <Windows.h>
#include <cstdlib>

#include "Memory.h"

Sound::Sound(Memory* memory)
    : m_memory(memory)
{
    XAudio2Create(m_audioEngine.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
    m_audioEngine->CreateMasteringVoice(&m_masteringVoice, 2, 44100);

    m_waveFormat =
    {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nChannels = 2,
        .nSamplesPerSec = 44100,
        .nAvgBytesPerSec = 44100 * 2,
        .nBlockAlign = 2, // (nChannels*wBitsPerSamaple)/8 for PCM formats
        .wBitsPerSample = 8,
        .cbSize = 0,
    };

    m_audioEngine->CreateSourceVoice(&m_squareWave1, &m_waveFormat);

    m_squareWave1->Start();
}

Sound::~Sound()
{
    m_masteringVoice->DestroyVoice();
    m_squareWave1->DestroyVoice();
}

static uint64_t totalSamplesGenerated = 0;
bool high = false;
void Sound::update(double deltaTimeSeconds)
{
    XAUDIO2_VOICE_STATE state;
    m_squareWave1->GetState(&state);
    if (state.BuffersQueued > 0) return;

    uint32_t numSamplesToGenerate = static_cast<uint32_t>(44100 * deltaTimeSeconds);

    uint8_t* buffer = nullptr;

    for (uint32_t i = 0; i < numSamplesToGenerate; i++)
    {
        if (totalSamplesGenerated % 261 == 0) high = !high;
        buffer[i] = high ? 10 : 0;
        totalSamplesGenerated++;
    }

    XAUDIO2_BUFFER xaudioBuffer =
    {
        .Flags = 0,
        .AudioBytes = 44100,
        .pAudioData = buffer,
        .PlayBegin = 0,
        .PlayLength = numSamplesToGenerate,
        .LoopBegin = 0,
        .LoopLength = 0,
        .LoopCount = XAUDIO2_NO_LOOP_REGION,
        .pContext = 0,
    };

    m_squareWave1->SubmitSourceBuffer(&xaudioBuffer);
}

void VoiceCallback::OnBufferEnd(void*)
{
    SetEvent(m_bufferEndEvent);
}

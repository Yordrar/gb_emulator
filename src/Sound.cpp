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

    m_squareWave1Callback = new VoiceCallback(m_memory, &m_squareWave1, VoiceCallback::VoiceType::SquareWave1);
    m_squareWave2Callback = new VoiceCallback(m_memory, &m_squareWave2, VoiceCallback::VoiceType::SquareWave2);
    m_customWaveCallback = new VoiceCallback(m_memory, &m_customWave, VoiceCallback::VoiceType::CustomWave);
    m_noiseWaveCallback = new VoiceCallback(m_memory, &m_noiseWave, VoiceCallback::VoiceType::NoiseWave);

    m_audioEngine->CreateSourceVoice(&m_squareWave1, &m_waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, m_squareWave1Callback);
    m_audioEngine->CreateSourceVoice(&m_squareWave2, &m_waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, m_squareWave2Callback);
    m_audioEngine->CreateSourceVoice(&m_customWave, &m_waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, m_customWaveCallback);
    m_audioEngine->CreateSourceVoice(&m_noiseWave, &m_waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, m_noiseWaveCallback);

    XAUDIO2_BUFFER xaudioBuffer =
    {
        .Flags = 0,
        .AudioBytes = 1600,
        .pAudioData = m_initialAudioDataBuffer,
        .PlayBegin = 0,
        .PlayLength = 0,
        .LoopBegin = 0,
        .LoopLength = 0,
        .LoopCount = XAUDIO2_NO_LOOP_REGION,
        .pContext = 0,
    };
    m_squareWave1->SubmitSourceBuffer(&xaudioBuffer);
    m_squareWave1->SetVolume(0.0f);
    //m_squareWave1->Start();

    m_squareWave2->SubmitSourceBuffer(&xaudioBuffer);
    m_squareWave2->SetVolume(0.0f);
    m_squareWave2->Start();
}

Sound::~Sound()
{
    m_squareWave1->Stop();
    m_squareWave1->DestroyVoice();

    m_squareWave2->Stop();
    m_squareWave2->DestroyVoice();

    m_customWave->Stop();
    m_customWave->DestroyVoice();

    m_noiseWave->Stop();
    m_noiseWave->DestroyVoice();

    m_masteringVoice->DestroyVoice();
}

void Sound::update(uint64_t cyclesToEmulate)
{
    m_squareWave2Callback->update(cyclesToEmulate);
}

VoiceCallback::VoiceCallback(Memory* memory, IXAudio2SourceVoice** sourceVoice, VoiceType voiceType)
    : m_bufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
    , m_memory(memory)
    , m_sourceVoice(sourceVoice)
    , m_voiceType(voiceType)
{
}

VoiceCallback::~VoiceCallback()
{
    CloseHandle(m_bufferEndEvent);
}

void VoiceCallback::update(uint64_t cyclesToEmulate)
{
    switch (m_voiceType)
    {
    case VoiceType::SquareWave1:
    {
        SquareWave1Func(cyclesToEmulate);
    }
    break;
    case VoiceType::SquareWave2:
    {
        SquareWave2Func(cyclesToEmulate);
    }
    break;
    case VoiceType::CustomWave:
    {
        CustomWaveFunc(cyclesToEmulate);
    }
    break;
    case VoiceType::NoiseWave:
    {
        NoiseWaveFunc(cyclesToEmulate);
    }
    break;
    default:
        break;
    }
}

void VoiceCallback::SquareWave1Func(uint64_t cyclesToEmulate)
{
    uint8_t* audioDataBuffer = m_currentBuffer ? m_audioDataBuffer2 : m_audioDataBuffer1;
    uint64_t& audioDataBufferSampleCount = m_currentBuffer ? m_audioDataBuffer2SampleCount : m_audioDataBuffer1SampleCount;

    if (audioDataBufferSampleCount >= sc_AudioDataBufferSize) return;



    for (uint64_t i = audioDataBufferSampleCount; i < (audioDataBufferSampleCount + cyclesToEmulate) && i < sc_AudioDataBufferSize; i++)
    {
        if ((runnningSampleCount % 441) == 0) high = !high;
        audioDataBuffer[i] = high ? 10 : 0;
        runnningSampleCount++;
    }

    audioDataBufferSampleCount += cyclesToEmulate;
}

void VoiceCallback::SquareWave2Func(uint64_t cyclesToEmulate)
{
    uint8_t* audioDataBuffer = m_currentBuffer ? m_audioDataBuffer2 : m_audioDataBuffer1;
    uint64_t& audioDataBufferSampleCount = m_currentBuffer ? m_audioDataBuffer2SampleCount : m_audioDataBuffer1SampleCount;

    if (audioDataBufferSampleCount >= sc_AudioDataBufferSize) return;



    for (uint64_t i = audioDataBufferSampleCount; i < (audioDataBufferSampleCount + cyclesToEmulate) && i < sc_AudioDataBufferSize; i++)
    {
        if ((runnningSampleCount % 441) == 0) high = !high;
        audioDataBuffer[i] = high ? 10 : 0;
        runnningSampleCount++;
    }

    audioDataBufferSampleCount += cyclesToEmulate;
}

void VoiceCallback::CustomWaveFunc(uint64_t cyclesToEmulate)
{
}

void VoiceCallback::NoiseWaveFunc(uint64_t cyclesToEmulate)
{
}

void VoiceCallback::OnBufferEnd(void*)
{
    uint8_t* audioDataBuffer = m_currentBuffer ? m_audioDataBuffer2 : m_audioDataBuffer1;
    uint64_t audioDataBufferSampleCount = m_currentBuffer ? m_audioDataBuffer2SampleCount : m_audioDataBuffer1SampleCount;
    XAUDIO2_BUFFER xaudioBuffer =
    {
        .Flags = 0,
        .AudioBytes = sc_AudioDataBufferSize,
        .pAudioData = audioDataBuffer,
        .PlayBegin = 0,
        .PlayLength = 0,
        .LoopBegin = 0,
        .LoopLength = 0,
        .LoopCount = XAUDIO2_NO_LOOP_REGION,
        .pContext = 0,
    };
    (*m_sourceVoice)->SetVolume(1.0f);
    (*m_sourceVoice)->SubmitSourceBuffer(&xaudioBuffer);
    m_currentBuffer = !m_currentBuffer;
    m_audioDataBuffer1SampleCount = 0;
    m_audioDataBuffer2SampleCount = 0;
}

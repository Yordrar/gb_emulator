#pragma once

#include <Windows.h>
#include <Windowsx.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#include <xaudio2.h>

#include <cstdint>

class Memory;

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    enum VoiceType
    {
        SquareWave1,
        SquareWave2,
        CustomWave,
        NoiseWave,
    };

    VoiceCallback(Memory* memory, IXAudio2SourceVoice** sourceVoice, VoiceType voiceType);
    ~VoiceCallback();

    void update(uint64_t cyclesToEmulate);

    void SquareWave1Func(uint64_t cyclesToEmulate);
    void SquareWave2Func(uint64_t cyclesToEmulate);
    void CustomWaveFunc(uint64_t cyclesToEmulate);
    void NoiseWaveFunc(uint64_t cyclesToEmulate);

    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
    void OnVoiceProcessingPassEnd() {}
    void OnStreamEnd() {}
    void OnBufferStart(void* pBufferContext) {}
    void OnBufferEnd(void*);
    void OnLoopEnd(void* pBufferContext) {}
    void OnVoiceError(void* pBufferContext, HRESULT Error) {}

private:
    HANDLE m_bufferEndEvent;

    Memory* m_memory;
    IXAudio2SourceVoice** m_sourceVoice;
    VoiceType m_voiceType;

    bool m_currentBuffer = 0;
    static const uint64_t sc_AudioDataBufferSize = 1600;
    uint8_t m_audioDataBuffer1[sc_AudioDataBufferSize] = { 0 };
    uint64_t m_audioDataBuffer1SampleCount = 0;
    uint8_t m_audioDataBuffer2[sc_AudioDataBufferSize] = { 0 };
    uint64_t m_audioDataBuffer2SampleCount = 0;

    uint64_t runnningSampleCount = 0;
    bool high = false;
};

class Sound
{
public:
    Sound(Memory* memory);
    ~Sound();

    void update(uint64_t cyclesToEmulate);

private:
    Memory* m_memory;

    ComPtr<IXAudio2> m_audioEngine;
    IXAudio2MasteringVoice* m_masteringVoice;

    IXAudio2SourceVoice* m_squareWave1;
    VoiceCallback* m_squareWave1Callback;

    IXAudio2SourceVoice* m_squareWave2;
    VoiceCallback* m_squareWave2Callback;

    IXAudio2SourceVoice* m_customWave;
    VoiceCallback* m_customWaveCallback;

    IXAudio2SourceVoice* m_noiseWave;
    VoiceCallback* m_noiseWaveCallback;

    WAVEFORMATEX m_waveFormat;

    uint8_t m_initialAudioDataBuffer[1600] = { 0 };
};
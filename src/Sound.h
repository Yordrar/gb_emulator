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
    VoiceCallback() : m_bufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
    ~VoiceCallback() { CloseHandle(m_bufferEndEvent); }
    void OnBufferEnd(void*);

    void OnVoiceProcessingPassEnd() {}
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
    void OnBufferStart(void* pBufferContext) {}
    void OnLoopEnd(void* pBufferContext) {}
    void OnVoiceError(void* pBufferContext, HRESULT Error) {}
private:
    HANDLE m_bufferEndEvent;
};

class Sound
{
public:
    Sound(Memory* memory);
    ~Sound();

    void update(double deltaTimeSeconds);

private:
    Memory* m_memory;

    ComPtr<IXAudio2> m_audioEngine;
    IXAudio2MasteringVoice* m_masteringVoice;
    IXAudio2SourceVoice* m_squareWave1;
    IXAudio2SourceVoice* m_squareWave2;
    IXAudio2SourceVoice* m_customWave;
    IXAudio2SourceVoice* m_noise;
    WAVEFORMATEX m_waveFormat;
};
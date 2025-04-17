#include "audio_capture.h"
#include "logging.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <vector>
#include <windows.h>
#include <combaseapi.h>
#include "audio_analysis.h"

void StartAudioCaptureThread(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data) {
    running = true;
    thread = std::thread([&]() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            LogToFile("Audio Thread: Failed to initialize COM: " + std::to_string(hr));
            running = false;
            return;
        }
        IMMDeviceEnumerator* pEnumerator = nullptr;
        IMMDevice* pDevice = nullptr;
        IAudioClient* pAudioClient = nullptr;
        IAudioCaptureClient* pCaptureClient = nullptr;
        WAVEFORMATEX* pwfx = nullptr;
        HANDLE hAudioEvent = nullptr;
        UINT32 bufferFrameCount = 0;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (FAILED(hr)) goto Exit;
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        if (FAILED(hr)) goto Exit;
        hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
        if (FAILED(hr)) goto Exit;
        hr = pAudioClient->GetMixFormat(&pwfx);
        if (FAILED(hr)) goto Exit;
        bool isFloatFormat = false;
        if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) isFloatFormat = true;
        else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            WAVEFORMATEXTENSIBLE* pwfex = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
            if (pwfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) isFloatFormat = true;
        }
        hAudioEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!hAudioEvent) goto Exit;
        REFERENCE_TIME hnsRequestedDuration = 200000;
        hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsRequestedDuration, 0, pwfx, nullptr);
        if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
            hr = pAudioClient->GetBufferSize(&bufferFrameCount);
            if (FAILED(hr)) goto Exit;
            hnsRequestedDuration = (REFERENCE_TIME)((10000.0 * 1000 / pwfx->nSamplesPerSec * bufferFrameCount) + 0.5);
            hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsRequestedDuration, 0, pwfx, nullptr);
        }
        if (FAILED(hr)) goto Exit;
        hr = pAudioClient->GetBufferSize(&bufferFrameCount);
        if (FAILED(hr)) goto Exit;
        hr = pAudioClient->SetEventHandle(hAudioEvent);
        if (FAILED(hr)) goto Exit;
        hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
        if (FAILED(hr)) goto Exit;
        hr = pAudioClient->Start();
        if (FAILED(hr)) goto Exit;
        while (running.load()) {
            DWORD waitResult = WaitForSingleObject(hAudioEvent, 200);
            if (!running.load()) break;
            if (waitResult == WAIT_OBJECT_0) {
                BYTE* pData = nullptr;
                UINT32 numFramesAvailable = 0;
                DWORD flags = 0;
                UINT64 devicePosition = 0;
                UINT64 qpcPosition = 0;
                hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, &devicePosition, &qpcPosition);
                if (SUCCEEDED(hr)) {
                    if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && pData && numFramesAvailable > 0 && isFloatFormat) {
                        AnalyzeAudioBuffer(reinterpret_cast<float*>(pData), numFramesAvailable, pwfx->nChannels, config, data);
                    }
                    pCaptureClient->ReleaseBuffer(numFramesAvailable);
                }
            }
        }
    Exit:
        if (pAudioClient) pAudioClient->Stop();
        if (hAudioEvent) CloseHandle(hAudioEvent);
        if (pCaptureClient) pCaptureClient->Release();
        if (pAudioClient) pAudioClient->Release();
        if (pDevice) pDevice->Release();
        if (pEnumerator) pEnumerator->Release();
        if (pwfx) CoTaskMemFree(pwfx);
        CoUninitialize();
        running = false;
    });
}
void StopAudioCaptureThread(std::atomic_bool& running, std::thread& thread) {
    running = false;
    if (thread.joinable()) thread.join();
}

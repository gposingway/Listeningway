// ---------------------------------------------
// Audio Capture Module Implementation
// Captures system audio using WASAPI loopback and feeds it to analysis
// ---------------------------------------------
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
#include <string>
#include "audio_analysis.h"
#include "settings.h"

// Globals for device notification
static std::atomic_bool g_device_change_pending = false;

// Notification client for device changes
class ListeningwayDeviceNotificationClient : public IMMNotificationClient {
    LONG _ref = 1;
public:
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&_ref); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ref = InterlockedDecrement(&_ref);
        if (ref == 0) delete this;
        return ref;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient)) {
            *ppvObject = static_cast<IMMNotificationClient*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override {
        if (flow == eRender && role == eConsole) {
            g_device_change_pending = true;
        }
        return S_OK;
    }
    // Unused notifications
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }
};

// Globals for device notification
static IMMDeviceEnumerator* g_device_enumerator = nullptr;
static ListeningwayDeviceNotificationClient* g_notification_client = nullptr;

void InitAudioDeviceNotification() {
    if (!g_device_enumerator) {
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&g_device_enumerator);
        if (FAILED(hr)) g_device_enumerator = nullptr;
    }
    if (g_device_enumerator && !g_notification_client) {
        g_notification_client = new ListeningwayDeviceNotificationClient();
        g_device_enumerator->RegisterEndpointNotificationCallback(g_notification_client);
    }
}

void UninitAudioDeviceNotification() {
    if (g_device_enumerator && g_notification_client) {
        g_device_enumerator->UnregisterEndpointNotificationCallback(g_notification_client);
        g_notification_client->Release();
        g_notification_client = nullptr;
    }
    if (g_device_enumerator) {
        g_device_enumerator->Release();
        g_device_enumerator = nullptr;
    }
}

// Starts a background thread that captures audio and updates analysis data.
void StartAudioCaptureThread(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data) {
    running = true;
    g_device_change_pending = false;
    LOG_DEBUG("[AudioCapture] Starting audio capture thread.");
    thread = std::thread([&, config]() {
        try {
            struct WasapiResources {
                IMMDevice* pDevice = nullptr;
                IAudioClient* pAudioClient = nullptr;
                IAudioCaptureClient* pCaptureClient = nullptr;
                WAVEFORMATEX* pwfx = nullptr;
                HANDLE hAudioEvent = nullptr;
                ~WasapiResources() {
                    if (pAudioClient) pAudioClient->Stop();
                    if (hAudioEvent) CloseHandle(hAudioEvent);
                    if (pCaptureClient) pCaptureClient->Release();
                    if (pAudioClient) pAudioClient->Release();
                    if (pDevice) pDevice->Release();
                    if (pwfx) CoTaskMemFree(pwfx);
                }
            } res;
            HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (FAILED(hr)) {
                LOG_ERROR("[AudioCapture] Failed to initialize COM: " + std::to_string(hr));
                running = false;
                return;
            }
            // WASAPI device and client setup
            UINT32 bufferFrameCount = 0;
            if (!g_device_enumerator) {
                LOG_ERROR("[AudioCapture] Device enumerator is null!");
                running = false;
                CoUninitialize();
                return;
            }
            hr = g_device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &res.pDevice);
            if (FAILED(hr)) {
                LOG_ERROR("[AudioCapture] Failed to get default audio endpoint: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            hr = res.pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&res.pAudioClient);
            if (FAILED(hr)) {
                LOG_ERROR("[AudioCapture] Failed to activate audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            hr = res.pAudioClient->GetMixFormat(&res.pwfx);
            if (FAILED(hr)) {
                LOG_ERROR("[AudioCapture] Failed to get mix format: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            // Check for float format
            bool isFloatFormat = false;
            if (res.pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) isFloatFormat = true;
            else if (res.pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                WAVEFORMATEXTENSIBLE* pwfex = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(res.pwfx);
                if (pwfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) isFloatFormat = true;
            }
            res.hAudioEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (!res.hAudioEvent) {
                LOG_ERROR("[AudioCapture] Failed to create audio event.");
                running = false;
                CoUninitialize();
                return;
            }
            REFERENCE_TIME hnsRequestedDuration = 200000;
            hr = res.pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsRequestedDuration, 0, res.pwfx, nullptr);
            if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
                hr = res.pAudioClient->GetBufferSize(&bufferFrameCount);
                if (FAILED(hr)) {
                    LOG_ERROR("[AudioCapture] Failed to get buffer size: " + std::to_string(hr));
                    running = false;
                    CoUninitialize();
                    return;
                }
                hnsRequestedDuration = (REFERENCE_TIME)((10000.0 * 1000 / res.pwfx->nSamplesPerSec * bufferFrameCount) + 0.5);
                hr = res.pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsRequestedDuration, 0, res.pwfx, nullptr);
            }
            if (FAILED(hr)) {
                LOG_ERROR("[AudioCapture] Failed to initialize audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            hr = res.pAudioClient->GetBufferSize(&bufferFrameCount);
            if (FAILED(hr)) {
                LOG_ERROR("[AudioCapture] Failed to get buffer size (2): " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            hr = res.pAudioClient->SetEventHandle(res.hAudioEvent);
            if (FAILED(hr)) {
                LOG_ERROR("[AudioCapture] Failed to set event handle: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            hr = res.pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&res.pCaptureClient);
            if (FAILED(hr)) {
                LOG_ERROR("[AudioCapture] Failed to get capture client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            hr = res.pAudioClient->Start();
            if (FAILED(hr)) {
                LOG_ERROR("[AudioCapture] Failed to start audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            LOG_DEBUG("[AudioCapture] Entering main capture loop.");
            // Main capture loop
            while (running.load()) {
                if (g_device_change_pending.load()) {
                    LOG_DEBUG("[AudioCapture] Audio device change detected. Restarting capture.");
                    break; // Exit thread to allow restart
                }
                DWORD waitResult = WaitForSingleObject(res.hAudioEvent, 200);
                if (!running.load()) break;
                if (waitResult == WAIT_OBJECT_0) {
                    BYTE* pData = nullptr;
                    UINT32 numFramesAvailable = 0;
                    DWORD flags = 0;
                    UINT64 devicePosition = 0;
                    UINT64 qpcPosition = 0;
                    hr = res.pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, &devicePosition, &qpcPosition);
                    if (SUCCEEDED(hr)) {
                        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && pData && numFramesAvailable > 0 && isFloatFormat) {
                            if (g_settings.audio_analysis_enabled) {
                                AnalyzeAudioBuffer(reinterpret_cast<float*>(pData), numFramesAvailable, res.pwfx->nChannels, config, data);
                            } else {
                                data.volume = 0.0f;
                                std::fill(data.freq_bands.begin(), data.freq_bands.end(), 0.0f);
                                data.beat = 0.0f;
                            }
                        }
                        res.pCaptureClient->ReleaseBuffer(numFramesAvailable);
                    } else {
                        LOG_ERROR("[AudioCapture] GetBuffer failed: " + std::to_string(hr));
                    }
                }
            }
            LOG_DEBUG("[AudioCapture] Exiting capture loop.");
            CoUninitialize();
            running = false;
            LOG_DEBUG("[AudioCapture] Audio capture thread stopped.");
        } catch (const std::exception& ex) {
            LOG_ERROR(std::string("[AudioCapture] Exception in capture thread: ") + ex.what());
            running = false;
        } catch (...) {
            LOG_ERROR("[AudioCapture] Unknown exception in capture thread.");
            running = false;
        }
    });
}

// Signals the capture thread to stop and joins it.
void StopAudioCaptureThread(std::atomic_bool& running, std::thread& thread) {
    running = false;
    if (thread.joinable()) thread.join();
}

// Helper to restart audio capture if device changed
void CheckAndRestartAudioCapture(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data) {
    if (g_device_change_pending.load()) {
        StopAudioCaptureThread(running, thread);
        StartAudioCaptureThread(config, running, thread, data_mutex, data);
        g_device_change_pending = false;
    }
}

// ---------------------------------------------
// System Audio Capture Provider Implementation
// Captures system-wide audio using WASAPI loopback
// ---------------------------------------------
#include "system_audio_provider.h"
#include "../audio_analysis.h"
#include "../../utils/logging.h"
#include "../../core/thread_safety_manager.h"
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <vector>
#include <windows.h>
#include <combaseapi.h>

// Static member definitions
std::atomic_bool SystemAudioCaptureProvider::device_change_pending_(false);
IMMDeviceEnumerator* SystemAudioCaptureProvider::device_enumerator_ = nullptr;
SystemAudioCaptureProvider::DeviceNotificationClient* SystemAudioCaptureProvider::notification_client_ = nullptr;

// Notification client for device changes
class SystemAudioCaptureProvider::DeviceNotificationClient : public IMMNotificationClient {
    LONG ref_ = 1;
public:
    ULONG STDMETHODCALLTYPE AddRef() override { 
        return InterlockedIncrement(&ref_); 
    }
    
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ref = InterlockedDecrement(&ref_);
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
            SystemAudioCaptureProvider::SetDeviceChangePending();
        }
        return S_OK;
    }
    
    // Unused notifications
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }
};

bool SystemAudioCaptureProvider::IsAvailable() const {
    // WASAPI is available on Windows Vista and later
    return true;
}

bool SystemAudioCaptureProvider::Initialize() {
    if (!device_enumerator_) {
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, 
                                    CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), 
                                    (void**)&device_enumerator_);
        if (FAILED(hr)) {
            LOG_ERROR("[SystemAudioProvider] Failed to create device enumerator: " + std::to_string(hr));
            return false;
        }
    }
    
    if (device_enumerator_ && !notification_client_) {
        notification_client_ = new DeviceNotificationClient();
        HRESULT hr = device_enumerator_->RegisterEndpointNotificationCallback(notification_client_);
        if (FAILED(hr)) {
            LOG_ERROR("[SystemAudioProvider] Failed to register notification callback: " + std::to_string(hr));
            notification_client_->Release();
            notification_client_ = nullptr;
            return false;
        }
    }
    
    device_change_pending_ = false;
    LOG_DEBUG("[SystemAudioProvider] Initialized successfully.");
    return true;
}

void SystemAudioCaptureProvider::Uninitialize() {
    if (device_enumerator_ && notification_client_) {
        device_enumerator_->UnregisterEndpointNotificationCallback(notification_client_);
        notification_client_->Release();
        notification_client_ = nullptr;
    }
    
    if (device_enumerator_) {
        device_enumerator_->Release();
        device_enumerator_ = nullptr;
    }
    
    LOG_DEBUG("[SystemAudioProvider] Uninitialized.");
}

bool SystemAudioCaptureProvider::StartCapture(const Listeningway::Configuration& config, 
                                             std::atomic_bool& running, 
                                             std::thread& thread, 
                                             AudioAnalysisData& data) {
    running = true;
    device_change_pending_ = false;
    LOG_DEBUG("[SystemAudioProvider] Starting audio capture thread.");
    
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
                LOG_ERROR("[SystemAudioProvider] Failed to initialize COM: " + std::to_string(hr));
                running = false;
                return;
            }
            
            // WASAPI device and client setup
            UINT32 bufferFrameCount = 0;
            
            if (!device_enumerator_) {
                LOG_ERROR("[SystemAudioProvider] Device enumerator is null!");
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = device_enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &res.pDevice);
            if (FAILED(hr)) {
                LOG_ERROR("[SystemAudioProvider] Failed to get default audio endpoint: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&res.pAudioClient);
            if (FAILED(hr)) {
                LOG_ERROR("[SystemAudioProvider] Failed to activate audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->GetMixFormat(&res.pwfx);
            if (FAILED(hr)) {
                LOG_ERROR("[SystemAudioProvider] Failed to get mix format: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            // Check for float format
            bool isFloatFormat = false;
            if (res.pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
                isFloatFormat = true;
            } else if (res.pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                WAVEFORMATEXTENSIBLE* pwfex = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(res.pwfx);
                if (pwfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
                    isFloatFormat = true;
                }
            }
            
            res.hAudioEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (!res.hAudioEvent) {
                LOG_ERROR("[SystemAudioProvider] Failed to create audio event.");
                running = false;
                CoUninitialize();
                return;
            }
            
            REFERENCE_TIME hnsRequestedDuration = 200000;
            hr = res.pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 
                                            AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 
                                            hnsRequestedDuration, 0, res.pwfx, nullptr);
            
            if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
                hr = res.pAudioClient->GetBufferSize(&bufferFrameCount);
                if (FAILED(hr)) {
                    LOG_ERROR("[SystemAudioProvider] Failed to get buffer size: " + std::to_string(hr));
                    running = false;
                    CoUninitialize();
                    return;
                }
                hnsRequestedDuration = (REFERENCE_TIME)((10000.0 * 1000 / res.pwfx->nSamplesPerSec * bufferFrameCount) + 0.5);
                hr = res.pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 
                                                AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 
                                                hnsRequestedDuration, 0, res.pwfx, nullptr);
            }
            
            if (FAILED(hr)) {
                LOG_ERROR("[SystemAudioProvider] Failed to initialize audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->GetBufferSize(&bufferFrameCount);
            if (FAILED(hr)) {
                LOG_ERROR("[SystemAudioProvider] Failed to get buffer size (2): " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->SetEventHandle(res.hAudioEvent);
            if (FAILED(hr)) {
                LOG_ERROR("[SystemAudioProvider] Failed to set event handle: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&res.pCaptureClient);
            if (FAILED(hr)) {
                LOG_ERROR("[SystemAudioProvider] Failed to get capture client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->Start();
            if (FAILED(hr)) {
                LOG_ERROR("[SystemAudioProvider] Failed to start audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            LOG_DEBUG("[SystemAudioProvider] Entering main capture loop.");
            
            // Main capture loop
            while (running.load()) {
                if (device_change_pending_.load()) {
                    LOG_DEBUG("[SystemAudioProvider] Audio device change detected. Restarting capture.");
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
                    
                    hr = res.pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, &devicePosition, &qpcPosition);                    if (SUCCEEDED(hr)) {                        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && pData && numFramesAvailable > 0 && isFloatFormat) {
                            // Check if analysis is enabled in config (thread-safe snapshot)
                            if (!Listeningway::ConfigurationManager::Snapshot().audio.analysisEnabled) {
                                res.pCaptureClient->ReleaseBuffer(numFramesAvailable);
                                continue;
                            }
                            
                            // Use centralized thread safety for audio data access
                            {
                                LOCK_AUDIO_DATA();
                                extern AudioAnalyzer g_audio_analyzer;
                                g_audio_analyzer.AnalyzeAudioBuffer(reinterpret_cast<float*>(pData), 
                                                                  numFramesAvailable, 
                                                                  res.pwfx->nChannels, 
                                                                  data);
                            }
                        }
                        res.pCaptureClient->ReleaseBuffer(numFramesAvailable);
                    } else {
                        LOG_ERROR("[SystemAudioProvider] GetBuffer failed: " + std::to_string(hr));
                    }
                }
            }
            
            LOG_DEBUG("[SystemAudioProvider] Exiting capture loop.");
            CoUninitialize();
            running = false;
            LOG_DEBUG("[SystemAudioProvider] Audio capture thread stopped.");
            
        } catch (const std::exception& ex) {
            LOG_ERROR(std::string("[SystemAudioProvider] Exception in capture thread: ") + ex.what());
            running = false;
        } catch (...) {
            LOG_ERROR("[SystemAudioProvider] Unknown exception in capture thread.");
            running = false;
        }
    });
    
    return true;
}

void SystemAudioCaptureProvider::StopCapture(std::atomic_bool& running, std::thread& thread) {
    running = false;
    if (thread.joinable()) {
        thread.join();
    }
}

AudioProviderInfo SystemAudioCaptureProvider::GetProviderInfo() const {
    return AudioProviderInfo{
        "system", // code as string
        "System Audio", // name
        true, // is_default
        2, // order
        true // activates_capture
    };
}

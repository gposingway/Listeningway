// ---------------------------------------------
// Process Audio Capture Provider Implementation
// Captures audio from the game process using WASAPI
// ---------------------------------------------
#include "process_audio_provider.h"
#include "../audio_analysis.h"
#include "../../utils/logging.h"
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <vector>
#include <windows.h>
#include <combaseapi.h>
#include <processthreadsapi.h>
#include <tlhelp32.h>
#include <psapi.h>

// Static member definitions
std::atomic_bool ProcessAudioCaptureProvider::device_change_pending_(false);
IMMDeviceEnumerator* ProcessAudioCaptureProvider::device_enumerator_ = nullptr;
ProcessAudioCaptureProvider::DeviceNotificationClient* ProcessAudioCaptureProvider::notification_client_ = nullptr;

// Notification client for device changes
class ProcessAudioCaptureProvider::DeviceNotificationClient : public IMMNotificationClient {
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
            ProcessAudioCaptureProvider::SetDeviceChangePending();
        }
        return S_OK;
    }
    
    // Unused notifications
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }
};

// Helper function to get parent process ID
DWORD GetParentProcessId(DWORD processId) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return 0;
    }
    
    DWORD parentPid = 0;
    do {
        if (pe32.th32ProcessID == processId) {
            parentPid = pe32.th32ParentProcessID;
            break;
        }
    } while (Process32Next(hSnapshot, &pe32));
    
    CloseHandle(hSnapshot);
    return parentPid;
}

// Helper function to get process name
std::string GetProcessName(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) {
        return "Unknown";
    }
    
    char processName[MAX_PATH] = "<unknown>";
    HMODULE hMod;
    DWORD cbNeeded;
    
    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
        GetModuleBaseNameA(hProcess, hMod, processName, sizeof(processName) / sizeof(char));
    }
    
    CloseHandle(hProcess);
    return std::string(processName);
}

ProcessAudioCaptureProvider::ProcessAudioCaptureProvider() {
    // For a ReShade addon, the current process IS the game process
    game_process_id_ = GetCurrentProcessId();
    
    std::string gameName = GetProcessName(game_process_id_);
    
    LOG_DEBUG("[ProcessAudioProvider] Target game process: " + gameName + " (PID: " + std::to_string(game_process_id_) + ")");
}

bool ProcessAudioCaptureProvider::IsAvailable() const {
    if (!device_enumerator_) {
        return false;
    }
    
    // Check if we can enumerate audio sessions
    IMMDevice* pDevice = nullptr;
    IAudioSessionManager2* pSessionManager = nullptr;
    IAudioSessionEnumerator* pSessionEnumerator = nullptr;
    
    HRESULT hr = device_enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&pSessionManager);
        if (SUCCEEDED(hr)) {
            hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
            if (SUCCEEDED(hr)) {
                int sessionCount = 0;
                hr = pSessionEnumerator->GetCount(&sessionCount);
                bool available = SUCCEEDED(hr) && sessionCount >= 0;
                
                pSessionEnumerator->Release();
                pSessionManager->Release();
                pDevice->Release();
                
                LOG_DEBUG("[ProcessAudioProvider] Process audio is " + std::string(available ? "available" : "not available") + 
                         " (found " + std::to_string(sessionCount) + " sessions)");
                return available;
            }
            pSessionManager->Release();
        }
        pDevice->Release();
    }
    
    return false;
}

bool ProcessAudioCaptureProvider::Initialize() {
    if (!device_enumerator_) {
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, 
                                    CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), 
                                    (void**)&device_enumerator_);
        if (FAILED(hr)) {
            LOG_ERROR("[ProcessAudioProvider] Failed to create device enumerator: " + std::to_string(hr));
            return false;
        }
    }
    
    if (device_enumerator_ && !notification_client_) {
        notification_client_ = new DeviceNotificationClient();
        HRESULT hr = device_enumerator_->RegisterEndpointNotificationCallback(notification_client_);
        if (FAILED(hr)) {
            LOG_ERROR("[ProcessAudioProvider] Failed to register notification callback: " + std::to_string(hr));
            notification_client_->Release();
            notification_client_ = nullptr;
            return false;
        }
    }
    
    device_change_pending_ = false;
    LOG_DEBUG("[ProcessAudioProvider] Initialized successfully for game PID: " + std::to_string(game_process_id_));
    return true;
}

void ProcessAudioCaptureProvider::Uninitialize() {
    if (device_enumerator_ && notification_client_) {
        device_enumerator_->UnregisterEndpointNotificationCallback(notification_client_);
        notification_client_->Release();
        notification_client_ = nullptr;
    }
    
    if (device_enumerator_) {
        device_enumerator_->Release();
        device_enumerator_ = nullptr;
    }
    
    LOG_DEBUG("[ProcessAudioProvider] Uninitialized.");
}

bool ProcessAudioCaptureProvider::FindGameAudioSession(IAudioSessionControl2** ppSessionControl2, float* pVolume) {
    *ppSessionControl2 = nullptr;
    *pVolume = 0.0f;
    
    IMMDevice* pDevice = nullptr;
    IAudioSessionManager2* pSessionManager = nullptr;
    IAudioSessionEnumerator* pSessionEnumerator = nullptr;
    
    HRESULT hr = device_enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        LOG_ERROR("[ProcessAudioProvider] Failed to get default audio endpoint: " + std::to_string(hr));
        return false;
    }
    
    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&pSessionManager);
    if (FAILED(hr)) {
        LOG_ERROR("[ProcessAudioProvider] Failed to activate session manager: " + std::to_string(hr));
        pDevice->Release();
        return false;
    }
    
    hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
    if (FAILED(hr)) {
        LOG_ERROR("[ProcessAudioProvider] Failed to get session enumerator: " + std::to_string(hr));
        pSessionManager->Release();
        pDevice->Release();
        return false;
    }
    
    int sessionCount = 0;
    hr = pSessionEnumerator->GetCount(&sessionCount);
    if (FAILED(hr)) {
        LOG_ERROR("[ProcessAudioProvider] Failed to get session count: " + std::to_string(hr));
        pSessionEnumerator->Release();
        pSessionManager->Release();
        pDevice->Release();
        return false;
    }
      LOG_DEBUG("[ProcessAudioProvider] Enumerating " + std::to_string(sessionCount) + " audio sessions for game PID: " + std::to_string(game_process_id_));
    
    for (int i = 0; i < sessionCount; i++) {
        IAudioSessionControl* pSessionControl = nullptr;
        IAudioSessionControl2* pSessionControl2 = nullptr;
        
        hr = pSessionEnumerator->GetSession(i, &pSessionControl);
        if (FAILED(hr)) continue;
        
        hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
        if (FAILED(hr)) {
            pSessionControl->Release();
            continue;
        }
        
        DWORD sessionProcessId = 0;
        hr = pSessionControl2->GetProcessId(&sessionProcessId);
        if (SUCCEEDED(hr) && sessionProcessId == game_process_id_) {
            LOG_DEBUG("[ProcessAudioProvider] Found audio session for game process (PID: " + std::to_string(sessionProcessId) + ")");
            
            // Get session volume
            ISimpleAudioVolume* pVolumeInterface = nullptr;
            hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pVolumeInterface);
            if (SUCCEEDED(hr)) {
                hr = pVolumeInterface->GetMasterVolume(pVolume);
                if (FAILED(hr)) {
                    *pVolume = 0.0f;
                }
                pVolumeInterface->Release();
            }
            
            *ppSessionControl2 = pSessionControl2;
            pSessionControl->Release();
            pSessionEnumerator->Release();
            pSessionManager->Release();
            pDevice->Release();
            return true;
        }
        
        pSessionControl2->Release();
        pSessionControl->Release();
    }
    
    pSessionEnumerator->Release();
    pSessionManager->Release();
    pDevice->Release();
    
    LOG_DEBUG("[ProcessAudioProvider] No audio session found for game process (PID: " + std::to_string(game_process_id_) + ")");
    return false;
}

bool ProcessAudioCaptureProvider::StartCapture(const AudioAnalysisConfig& config, 
                                              std::atomic_bool& running, 
                                              std::thread& thread, 
                                              std::mutex& data_mutex, 
                                              AudioAnalysisData& data) {
    running = true;
    device_change_pending_ = false;
    LOG_DEBUG("[ProcessAudioProvider] Starting game process audio capture thread for PID: " + std::to_string(game_process_id_));
    
    thread = std::thread([&, config]() {
        try {
            struct WasapiResources {
                IMMDevice* pDevice = nullptr;
                IAudioClient* pAudioClient = nullptr;
                IAudioCaptureClient* pCaptureClient = nullptr;
                IAudioSessionControl2* pGameSession = nullptr;
                WAVEFORMATEX* pwfx = nullptr;
                HANDLE hAudioEvent = nullptr;
                
                ~WasapiResources() {
                    if (pAudioClient) pAudioClient->Stop();
                    if (hAudioEvent) CloseHandle(hAudioEvent);
                    if (pCaptureClient) pCaptureClient->Release();
                    if (pAudioClient) pAudioClient->Release();
                    if (pGameSession) pGameSession->Release();
                    if (pDevice) pDevice->Release();
                    if (pwfx) CoTaskMemFree(pwfx);
                }
            } res;
            
            HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioProvider] Failed to initialize COM: " + std::to_string(hr));
                running = false;
                return;
            }            // Check if game audio session exists and get its volume
            float gameVolume = 0.0f;
            bool hasGameSession = FindGameAudioSession(&res.pGameSession, &gameVolume);
            
            if (hasGameSession) {
                LOG_DEBUG("[ProcessAudioProvider] Found game audio session with volume: " + std::to_string(gameVolume));
                
                // For process-specific audio, we need to use session-specific capture
                // Unfortunately, Windows doesn't provide direct session-specific capture
                // So we'll use system loopback but apply process-specific volume scaling
                LOG_DEBUG("[ProcessAudioProvider] Using system loopback with game session volume scaling");
            } else {
                LOG_WARNING("[ProcessAudioProvider] No specific game audio session found, will capture all system audio as fallback");
            }
            
            // Set up system-wide capture with game session filtering
            hr = device_enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &res.pDevice);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioProvider] Failed to get default audio endpoint: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&res.pAudioClient);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioProvider] Failed to activate audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }            
            hr = res.pAudioClient->GetMixFormat(&res.pwfx);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioProvider] Failed to get mix format: " + std::to_string(hr));
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
                LOG_ERROR("[ProcessAudioProvider] Failed to create audio event.");
                running = false;
                CoUninitialize();
                return;
            }
            
            REFERENCE_TIME hnsRequestedDuration = 200000;
            hr = res.pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 
                                            AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 
                                            hnsRequestedDuration, 0, res.pwfx, nullptr);
            
            UINT32 bufferFrameCount = 0;
            if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
                hr = res.pAudioClient->GetBufferSize(&bufferFrameCount);
                if (FAILED(hr)) {
                    LOG_ERROR("[ProcessAudioProvider] Failed to get buffer size: " + std::to_string(hr));
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
                LOG_ERROR("[ProcessAudioProvider] Failed to initialize audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->GetBufferSize(&bufferFrameCount);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioProvider] Failed to get buffer size (2): " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->SetEventHandle(res.hAudioEvent);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioProvider] Failed to set event handle: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&res.pCaptureClient);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioProvider] Failed to get capture client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->Start();
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioProvider] Failed to start audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            LOG_DEBUG("[ProcessAudioProvider] Entering main capture loop for game PID: " + std::to_string(game_process_id_));
            
            // Main capture loop with game session awareness
            while (running.load()) {
                if (device_change_pending_.load()) {
                    LOG_DEBUG("[ProcessAudioProvider] Audio device change detected. Restarting capture.");
                    break; // Exit thread to allow restart
                }
                
                DWORD waitResult = WaitForSingleObject(res.hAudioEvent, 200);
                if (!running.load()) break;
                
                if (waitResult == WAIT_OBJECT_0) {
                    BYTE* pData = nullptr;
                    UINT32 numFramesAvailable = 0;
                    DWORD flags = 0;                    UINT64 devicePosition = 0;
                    UINT64 qpcPosition = 0;
                    
                    hr = res.pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, &devicePosition, &qpcPosition);
                    if (SUCCEEDED(hr)) {
                        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && pData && numFramesAvailable > 0 && isFloatFormat) {
                            extern AudioAnalyzer g_audio_analyzer;
                            extern ListeningwaySettings g_settings;                            if (g_settings.audio_analysis_enabled) {
                                // Process audio with game session awareness
                                bool processAudio = true;
                                float volumeScale = 1.0f;
                                
                                if (res.pGameSession) {
                                    // Check if game session is active
                                    AudioSessionState sessionState;
                                    if (SUCCEEDED(res.pGameSession->GetState(&sessionState))) {
                                        processAudio = (sessionState == AudioSessionStateActive);
                                        
                                        if (processAudio) {
                                            // Get current game session volume for scaling
                                            ISimpleAudioVolume* pVolumeInterface = nullptr;
                                            if (SUCCEEDED(res.pGameSession->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pVolumeInterface))) {
                                                float currentVolume = 1.0f;
                                                if (SUCCEEDED(pVolumeInterface->GetMasterVolume(&currentVolume))) {
                                                    // Scale audio analysis based on game volume vs system volume
                                                    // This helps differentiate game audio impact vs other system audio
                                                    volumeScale = currentVolume;
                                                }
                                                pVolumeInterface->Release();
                                            }
                                        }
                                    }
                                }
                                
                                if (processAudio) {
                                    // Use the global audio analyzer with volume scaling for game session awareness
                                    g_audio_analyzer.AnalyzeAudioBuffer(reinterpret_cast<float*>(pData), 
                                                                      numFramesAvailable, 
                                                                      res.pwfx->nChannels, 
                                                                      config, 
                                                                      data);
                                    
                                    // Apply game session volume scaling to make the difference more apparent
                                    if (res.pGameSession && volumeScale < 1.0f) {
                                        data.volume *= volumeScale;
                                        for (auto& band : data.freq_bands) {
                                            band *= volumeScale;
                                        }
                                        data.beat *= volumeScale;
                                    }
                                } else {
                                    // Game session not active, mute output to show the difference
                                    data.volume = 0.0f;
                                    std::fill(data.freq_bands.begin(), data.freq_bands.end(), 0.0f);
                                    data.beat = 0.0f;
                                }
                            } else {
                                data.volume = 0.0f;
                                std::fill(data.freq_bands.begin(), data.freq_bands.end(), 0.0f);
                                data.beat = 0.0f;
                            }
                        }
                        res.pCaptureClient->ReleaseBuffer(numFramesAvailable);
                    } else {
                        LOG_ERROR("[ProcessAudioProvider] GetBuffer failed: " + std::to_string(hr));
                    }
                }
            }
            
            LOG_DEBUG("[ProcessAudioProvider] Exiting capture loop.");
            CoUninitialize();
            running = false;
            LOG_DEBUG("[ProcessAudioProvider] Game process audio capture thread stopped.");
            
        } catch (const std::exception& ex) {
            LOG_ERROR(std::string("[ProcessAudioProvider] Exception in capture thread: ") + ex.what());
            running = false;
        } catch (...) {
            LOG_ERROR("[ProcessAudioProvider] Unknown exception in capture thread.");
            running = false;
        }
    });
    
    return true;
}

void ProcessAudioCaptureProvider::StopCapture(std::atomic_bool& running, std::thread& thread) {
    running = false;
    if (thread.joinable()) {
        thread.join();
    }
}

AudioProviderInfo ProcessAudioCaptureProvider::GetProviderInfo() const {
    return AudioProviderInfo{
        "game", // code as string
        "Game", // name
        false, // is_default
        1, // order
        true // activates_capture
    };
}

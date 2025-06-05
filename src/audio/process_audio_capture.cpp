// ---------------------------------------------
// Process-Specific Audio Capture Module Implementation
// Captures audio from only the current process using WASAPI Session Management
// ---------------------------------------------
#include "process_audio_capture.h"
#include "audio_capture.h"
#include "logging.h"
#include "settings.h"
#include "audio_analysis.h"
#include "../core/configuration/configuration_manager.h"
#include "../core/thread_safety_manager.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <vector>
#include <windows.h>
#include <combaseapi.h>
#include <string>
#include <processthreadsapi.h>

extern IMMDeviceEnumerator* g_device_enumerator;
extern AudioAnalyzer g_audio_analyzer;

/**
 * @brief Helper class to manage IAudioSessionControl2 interface
 */
class AudioSessionControl2Wrapper {
public:
    IAudioSessionControl2* session_control = nullptr;
    
    AudioSessionControl2Wrapper() = default;
    ~AudioSessionControl2Wrapper() {
        if (session_control) {
            session_control->Release();
            session_control = nullptr;
        }
    }
    
    // Non-copyable
    AudioSessionControl2Wrapper(const AudioSessionControl2Wrapper&) = delete;
    AudioSessionControl2Wrapper& operator=(const AudioSessionControl2Wrapper&) = delete;
};

HRESULT FindProcessAudioSession(IMMDeviceEnumerator* device_enumerator,
                               DWORD target_pid,
                               IAudioSessionControl** out_session_control,
                               IAudioClient** out_audio_client) {
    if (!device_enumerator || !out_session_control || !out_audio_client) {
        return E_INVALIDARG;
    }
    
    *out_session_control = nullptr;
    *out_audio_client = nullptr;
    
    // If target_pid is 0, use current process ID
    if (target_pid == 0) {
        target_pid = GetCurrentProcessId();
    }
    
    LOG_DEBUG("[ProcessAudioCapture] Looking for audio session for process ID: " + std::to_string(target_pid));
    
    IMMDevice* device = nullptr;
    HRESULT hr = device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) {
        LOG_ERROR("[ProcessAudioCapture] Failed to get default audio endpoint: " + std::to_string(hr));
        return hr;
    }
    
    IAudioSessionManager2* session_manager = nullptr;
    hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&session_manager);
    device->Release();
    
    if (FAILED(hr)) {
        LOG_ERROR("[ProcessAudioCapture] Failed to activate session manager: " + std::to_string(hr));
        return hr;
    }
    
    IAudioSessionEnumerator* session_enumerator = nullptr;
    hr = session_manager->GetSessionEnumerator(&session_enumerator);
    if (FAILED(hr)) {
        LOG_ERROR("[ProcessAudioCapture] Failed to get session enumerator: " + std::to_string(hr));
        session_manager->Release();
        return hr;
    }
    
    int session_count = 0;
    hr = session_enumerator->GetCount(&session_count);
    if (FAILED(hr)) {
        LOG_ERROR("[ProcessAudioCapture] Failed to get session count: " + std::to_string(hr));
        session_enumerator->Release();
        session_manager->Release();
        return hr;
    }
    
    LOG_DEBUG("[ProcessAudioCapture] Found " + std::to_string(session_count) + " audio sessions");
    
    for (int i = 0; i < session_count; i++) {
        IAudioSessionControl* session_control = nullptr;
        hr = session_enumerator->GetSession(i, &session_control);
        if (FAILED(hr)) continue;
        
        // Query for IAudioSessionControl2 to get process ID
        IAudioSessionControl2* session_control2 = nullptr;
        hr = session_control->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&session_control2);
        if (FAILED(hr)) {
            session_control->Release();
            continue;
        }
        
        DWORD session_pid = 0;
        hr = session_control2->GetProcessId(&session_pid);
        if (SUCCEEDED(hr) && session_pid == target_pid) {
            LOG_DEBUG("[ProcessAudioCapture] Found matching audio session for process " + std::to_string(target_pid));
            
            // Get the display name for debugging
            LPWSTR display_name = nullptr;
            if (SUCCEEDED(session_control2->GetDisplayName(&display_name)) && display_name) {
                LOG_DEBUG("[ProcessAudioCapture] Session display name: " + std::string((char*)display_name)); // Simplified conversion
                CoTaskMemFree(display_name);
            }
            
            // Try to get the audio client from the session
            // Note: This is a simplified approach. In practice, we might need to use
            // IAudioSessionControl::GetService or create a new client
            IMMDevice* session_device = nullptr;
            hr = device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &session_device);
            if (SUCCEEDED(hr)) {
                hr = session_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)out_audio_client);
                session_device->Release();
                
                if (SUCCEEDED(hr)) {
                    *out_session_control = session_control;
                    session_control2->Release();
                    session_enumerator->Release();
                    session_manager->Release();
                    return S_OK;
                }
            }
        }
        
        session_control2->Release();
        session_control->Release();
    }
    
    session_enumerator->Release();
    session_manager->Release();
    
    LOG_DEBUG("[ProcessAudioCapture] No matching audio session found for process " + std::to_string(target_pid));
    return E_FAIL;
}

bool IsProcessAudioCaptureAvailable() {
    if (!g_device_enumerator) {
        return false;
    }
    
    IAudioSessionControl* session_control = nullptr;
    IAudioClient* audio_client = nullptr;
    
    HRESULT hr = FindProcessAudioSession(g_device_enumerator, 0, &session_control, &audio_client);
    
    if (session_control) session_control->Release();
    if (audio_client) audio_client->Release();
    
    return SUCCEEDED(hr);
}

bool GetCurrentProcessAudioInfo(wchar_t* session_name, size_t session_name_size, DWORD* process_id) {
    if (!session_name || !process_id || !g_device_enumerator) {
        return false;
    }
    
    *process_id = GetCurrentProcessId();
    
    IAudioSessionControl* session_control = nullptr;
    IAudioClient* audio_client = nullptr;
    
    HRESULT hr = FindProcessAudioSession(g_device_enumerator, 0, &session_control, &audio_client);
    
    bool success = false;
    if (SUCCEEDED(hr) && session_control) {
        IAudioSessionControl2* session_control2 = nullptr;
        hr = session_control->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&session_control2);
        if (SUCCEEDED(hr)) {
            LPWSTR display_name = nullptr;
            if (SUCCEEDED(session_control2->GetDisplayName(&display_name)) && display_name) {
                wcsncpy_s(session_name, session_name_size, display_name, _TRUNCATE);
                CoTaskMemFree(display_name);
                success = true;
            }
            session_control2->Release();
        }
    }
    
    if (session_control) session_control->Release();
    if (audio_client) audio_client->Release();
    
    return success;
}

void StartProcessAudioCaptureThread(const Listeningway::Configuration& config, 
                                   const ProcessAudioCaptureConfig& process_config,
                                   std::atomic_bool& running, 
                                   std::thread& thread, 
                                   AudioAnalysisData& data) {
    running = true;
    LOG_DEBUG("[ProcessAudioCapture] Starting process-specific audio capture thread.");
    
    thread = std::thread([&, config, process_config]() {
        try {
            struct ProcessWasapiResources {
                IMMDevice* pDevice = nullptr;
                IAudioClient* pAudioClient = nullptr;
                IAudioCaptureClient* pCaptureClient = nullptr;
                IAudioSessionControl* pSessionControl = nullptr;
                WAVEFORMATEX* pwfx = nullptr;
                HANDLE hAudioEvent = nullptr;
                
                ~ProcessWasapiResources() {
                    if (pAudioClient) pAudioClient->Stop();
                    if (hAudioEvent) CloseHandle(hAudioEvent);
                    if (pCaptureClient) pCaptureClient->Release();
                    if (pAudioClient) pAudioClient->Release();
                    if (pSessionControl) pSessionControl->Release();
                    if (pDevice) pDevice->Release();
                    if (pwfx) CoTaskMemFree(pwfx);
                }
            } res;
            
            HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioCapture] Failed to initialize COM: " + std::to_string(hr));
                running = false;
                return;
            }
            
            if (!g_device_enumerator) {
                LOG_ERROR("[ProcessAudioCapture] Device enumerator is null!");
                running = false;
                CoUninitialize();
                return;
            }
            
            // Try to find process-specific audio session
            DWORD target_pid = process_config.target_process_id;
            if (target_pid == 0) {
                target_pid = GetCurrentProcessId();
            }
            
            auto start_time = std::chrono::steady_clock::now();
            bool found_process_session = false;
            
            // Try to find the process audio session with timeout
            while (running.load() && !found_process_session) {
                hr = FindProcessAudioSession(g_device_enumerator, target_pid, &res.pSessionControl, &res.pAudioClient);
                if (SUCCEEDED(hr)) {
                    found_process_session = true;
                    LOG_DEBUG("[ProcessAudioCapture] Successfully found process audio session");
                    break;
                }
                
                // Check timeout
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - start_time).count();
                if (elapsed >= process_config.fallback_timeout) {
                    LOG_DEBUG("[ProcessAudioCapture] Timeout waiting for process audio session, falling back to system audio");
                    break;
                }
                
                // Wait a bit before retrying
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            // If we couldn't find process-specific session, fall back to system audio
            if (!found_process_session || !res.pAudioClient) {
                LOG_DEBUG("[ProcessAudioCapture] Falling back to system-wide audio capture");
                if (res.pSessionControl) {
                    res.pSessionControl->Release();
                    res.pSessionControl = nullptr;
                }
                if (res.pAudioClient) {
                    res.pAudioClient->Release();
                    res.pAudioClient = nullptr;
                }
                
                // Use the existing system-wide capture method
                hr = g_device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &res.pDevice);
                if (FAILED(hr)) {
                    LOG_ERROR("[ProcessAudioCapture] Failed to get default audio endpoint: " + std::to_string(hr));
                    running = false;
                    CoUninitialize();
                    return;
                }
                
                hr = res.pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&res.pAudioClient);
                if (FAILED(hr)) {
                    LOG_ERROR("[ProcessAudioCapture] Failed to activate audio client: " + std::to_string(hr));
                    running = false;
                    CoUninitialize();
                    return;
                }
            }
            
            // Continue with standard WASAPI setup
            hr = res.pAudioClient->GetMixFormat(&res.pwfx);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioCapture] Failed to get mix format: " + std::to_string(hr));
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
                LOG_ERROR("[ProcessAudioCapture] Failed to create audio event.");
                running = false;
                CoUninitialize();
                return;
            }
            
            REFERENCE_TIME hnsRequestedDuration = 200000;
            UINT32 bufferFrameCount = 0;
            
            // For process-specific capture, we still use loopback mode but focus on the specific session
            DWORD stream_flags = AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
            hr = res.pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, stream_flags, hnsRequestedDuration, 0, res.pwfx, nullptr);
            
            if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
                hr = res.pAudioClient->GetBufferSize(&bufferFrameCount);
                if (FAILED(hr)) {
                    LOG_ERROR("[ProcessAudioCapture] Failed to get buffer size: " + std::to_string(hr));
                    running = false;
                    CoUninitialize();
                    return;
                }
                hnsRequestedDuration = (REFERENCE_TIME)((10000.0 * 1000 / res.pwfx->nSamplesPerSec * bufferFrameCount) + 0.5);
                hr = res.pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, stream_flags, hnsRequestedDuration, 0, res.pwfx, nullptr);
            }
            
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioCapture] Failed to initialize audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->GetBufferSize(&bufferFrameCount);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioCapture] Failed to get buffer size (2): " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->SetEventHandle(res.hAudioEvent);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioCapture] Failed to set event handle: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&res.pCaptureClient);
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioCapture] Failed to get capture client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            hr = res.pAudioClient->Start();
            if (FAILED(hr)) {
                LOG_ERROR("[ProcessAudioCapture] Failed to start audio client: " + std::to_string(hr));
                running = false;
                CoUninitialize();
                return;
            }
            
            LOG_DEBUG("[ProcessAudioCapture] Entering main capture loop.");
            
            // Main capture loop
            while (running.load()) {
                DWORD waitResult = WaitForSingleObject(res.hAudioEvent, 200);
                if (!running.load()) break;
                
                if (waitResult == WAIT_OBJECT_0) {
                    BYTE* pData = nullptr;
                    UINT32 numFramesAvailable = 0;
                    DWORD flags = 0;
                    UINT64 devicePosition = 0;
                    UINT64 qpcPosition = 0;
                    
                    hr = res.pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, &devicePosition, &qpcPosition);                    if (SUCCEEDED(hr)) {                        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && pData && numFramesAvailable > 0 && isFloatFormat) {
                            // Use centralized thread safety for audio data access
                            {
                                LOCK_AUDIO_DATA();
                                
                                if (Listeningway::ConfigurationManager::Snapshot().audio.analysisEnabled) { // Thread-safe snapshot for capture thread
                                    // Use the global audio analyzer
                                    g_audio_analyzer.AnalyzeAudioBuffer(reinterpret_cast<float*>(pData), 
                                                                      numFramesAvailable, 
                                                                      res.pwfx->nChannels, 
                                                                      data);
                                } else {
                                    data.volume = 0.0f;
                                    std::fill(data.freq_bands.begin(), data.freq_bands.end(), 0.0f);
                                    data.beat = 0.0f;
                                }
                            }
                        }
                        res.pCaptureClient->ReleaseBuffer(numFramesAvailable);
                    } else {
                        LOG_ERROR("[ProcessAudioCapture] GetBuffer failed: " + std::to_string(hr));
                    }
                }
            }
            
            LOG_DEBUG("[ProcessAudioCapture] Exiting capture loop.");
            CoUninitialize();
            running = false;
            LOG_DEBUG("[ProcessAudioCapture] Process audio capture thread stopped.");
            
        } catch (const std::exception& ex) {
            LOG_ERROR(std::string("[ProcessAudioCapture] Exception in capture thread: ") + ex.what());
            running = false;
        } catch (...) {
            LOG_ERROR("[ProcessAudioCapture] Unknown exception in capture thread.");
            running = false;
        }
    });
}

void StopProcessAudioCaptureThread(std::atomic_bool& running, std::thread& thread) {
    running = false;
    if (thread.joinable()) {
        thread.join();
    }
}

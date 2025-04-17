#include <reshade.hpp> // Moved back up
#include <windows.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream> // For logging
#include <string>  // For logging
#include <chrono>  // For sleep and logging timestamp
#include <cmath>   // For audio analysis later
#include <combaseapi.h> // For COM
#include <mmdeviceapi.h> // For WASAPI
#include <audioclient.h> // For WASAPI
#include <algorithm> // Required for std::min, std::copy

// Include KissFFT headers (for real FFT)
#include <kiss_fftr.h>
#include <kiss_fft.h> // Contains kiss_fft_scalar

// --- Simple Logger ---
std::ofstream g_log_file;
std::mutex g_log_mutex;

void LogToFile(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    if (g_log_file.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_s(&now_tm, &now_c); // Use localtime_s for safety
        char time_str[100];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &now_tm);
        g_log_file << "[" << time_str << "] " << message << std::endl;
    }
}

// --- Audio Capture State ---
std::thread g_audio_thread;
std::atomic_bool g_audio_thread_running = false;

// --- Global State ---
// Used to ensure we only register/unregister once
static std::atomic_bool g_addon_enabled = false;
static reshade::api::effect_runtime* g_runtime = nullptr; // Store the runtime globally
static std::mutex g_runtime_mutex; // Mutex to protect access to g_runtime

// --- Audio Analysis Data Structure ---
const size_t NUM_BANDS = 10; // Example number of frequency bands
struct AudioAnalysisData {
    float volume = 0.0f;
    std::vector<float> freq_bands;

    AudioAnalysisData() : freq_bands(NUM_BANDS, 0.0f) {}
};

// --- Shared Audio Data & Mutex ---
AudioAnalysisData g_audio_data;
std::mutex g_audio_data_mutex;

// --- Forward declarations ---
// ReShade event handlers
// Correct signature found via grep
static void on_reshade_present(reshade::api::command_queue* queue, reshade::api::swapchain* swapchain, const reshade::api::rect* source_rect, const reshade::api::rect* dest_rect, uint32_t dirty_rect_count, const reshade::api::rect* dirty_rects);
static void on_reshade_init_device(reshade::api::device* device);
static void on_reshade_destroy_device(reshade::api::device* device);
static void on_reshade_init_runtime(reshade::api::effect_runtime* runtime); // New handler
static void on_reshade_destroy_runtime(reshade::api::effect_runtime* runtime); // New handler
static void AudioCaptureThreadFunc();

// Helper functions for registration
static void RegisterListeningwayAddon();
static void UnregisterListeningwayAddon();

// --- DllMain ---
// Entry point for the DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved); // Not used

    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            // Disable thread library calls to avoid potential deadlocks
            // if the DLL is loaded under unusual circumstances.
            DisableThreadLibraryCalls(hModule);

            // Check if the host application is loading ReShade
            // We only want to register our addon if ReShade is present.
            // reshade::register_addon expects the hModule of the addon DLL.
            if (reshade::register_addon(hModule))
            {
                RegisterListeningwayAddon();
            }
            // If ReShade isn't present or registration fails, do nothing more.
            break;

        case DLL_PROCESS_DETACH:
            // Only unregister if we successfully registered before.
            if (g_addon_enabled.load())
            {
                UnregisterListeningwayAddon();
            }
            break;
    }
    return TRUE;
}

// --- Addon Registration ---

static void RegisterListeningwayAddon()
{
    // Open log file (append mode)
    g_log_file.open("listeningway.log", std::ios::out | std::ios::app);
    if (g_log_file.is_open()) {
        LogToFile("Addon loading...");
    } else {
        OutputDebugStringA("Listeningway Addon: Failed to open listeningway.log\n");
    }

    // Register event callbacks
    reshade::register_event<reshade::addon_event::init_device>(on_reshade_init_device);
    reshade::register_event<reshade::addon_event::present>(on_reshade_present);
    reshade::register_event<reshade::addon_event::destroy_device>(on_reshade_destroy_device);
    reshade::register_event<reshade::addon_event::init_effect_runtime>(on_reshade_init_runtime); // Register new handler
    reshade::register_event<reshade::addon_event::destroy_effect_runtime>(on_reshade_destroy_runtime); // Register new handler

    g_addon_enabled = true; // Mark as enabled *after* successful registration
    LogToFile("Addon registered successfully.");
}

static void UnregisterListeningwayAddon()
{
    LogToFile("Addon unregistering...");

    // Unregister events
    reshade::unregister_event<reshade::addon_event::init_device>(on_reshade_init_device);
    reshade::unregister_event<reshade::addon_event::destroy_device>(on_reshade_destroy_device);
    reshade::unregister_event<reshade::addon_event::present>(on_reshade_present);
    reshade::unregister_event<reshade::addon_event::init_effect_runtime>(on_reshade_init_runtime); // Unregister new handler
    reshade::unregister_event<reshade::addon_event::destroy_effect_runtime>(on_reshade_destroy_runtime); // Unregister new handler

    // Unregister the addon itself
    reshade::unregister_addon(nullptr);

    g_addon_enabled = false;

    // Stop audio thread if it's somehow still running (safety check)
    if (g_audio_thread_running.load()) {
        LogToFile("Stopping audio thread from UnregisterListeningwayAddon (unexpected)...");
        g_audio_thread_running = false;
        if (g_audio_thread.joinable()) {
            g_audio_thread.join();
        }
        LogToFile("Audio thread stopped.");
    }

    LogToFile("Addon unregistered.");
    if (g_log_file.is_open()) {
        g_log_file.close();
    }
}

// --- ReShade Event Handlers ---

static void on_reshade_init_runtime(reshade::api::effect_runtime* runtime)
{
    std::lock_guard<std::mutex> lock(g_runtime_mutex);
    if (g_runtime == nullptr) { // Store the first runtime we encounter
        g_runtime = runtime;
        LogToFile("Effect runtime initialized and stored.");
    } else {
        LogToFile("Another effect runtime initialized, but one is already stored.");
        // Handle multiple runtimes if necessary (e.g., store in a list)
    }
}

static void on_reshade_destroy_runtime(reshade::api::effect_runtime* runtime)
{
    std::lock_guard<std::mutex> lock(g_runtime_mutex);
    if (g_runtime == runtime) { // Clear the stored runtime if it's the one being destroyed
        g_runtime = nullptr;
        LogToFile("Stored effect runtime destroyed.");
    } else {
        LogToFile("An effect runtime destroyed, but it wasn't the one stored.");
        // Handle multiple runtimes if necessary (e.g., remove from a list)
    }
}

static void on_reshade_present(reshade::api::command_queue* queue, reshade::api::swapchain* swapchain, const reshade::api::rect* source_rect, const reshade::api::rect* dest_rect, uint32_t dirty_rect_count, const reshade::api::rect* dirty_rects)
{
    reshade::api::effect_runtime* runtime = nullptr;
    { // Scope for lock guard
        std::lock_guard<std::mutex> lock(g_runtime_mutex);
        runtime = g_runtime; // Get the stored runtime
    }

    if (!runtime) {
        return; // No runtime available
    }

    // Avoid updating uniforms multiple times per frame if VR or other multi-pass rendering is active
    // This check might need refinement for VR
    // if (eye != 0) { return; } // 'eye' is not available in this signature

    // Read the latest audio analysis data thread-safely
    float current_volume = 0.0f;
    std::vector<float> current_bands(NUM_BANDS, 0.0f); // Create a copy

    { // Scope for lock guard
        std::lock_guard<std::mutex> lock(g_audio_data_mutex);
        current_volume = g_audio_data.volume;
        size_t elements_to_copy = std::min(current_bands.size(), g_audio_data.freq_bands.size());
        std::copy(g_audio_data.freq_bands.begin(), g_audio_data.freq_bands.begin() + elements_to_copy, current_bands.begin());
    } // Mutex is released here

    // Find handles and update the ReShade FX uniforms on each frame using the stored runtime
    reshade::api::effect_uniform_variable volume_handle = runtime->find_uniform_variable("Listeningway.fx", "fListeningwayVolume");
    reshade::api::effect_uniform_variable bands_handle = runtime->find_uniform_variable("Listeningway.fx", "fListeningwayFreqBands");

    if (volume_handle != 0) { // Check if handle is valid
        runtime->set_uniform_value_float(volume_handle, &current_volume, 1);
    }
    if (bands_handle != 0) { // Check if handle is valid
        runtime->set_uniform_value_float(bands_handle, current_bands.data(), current_bands.size());
    }

    // Keep UNREFERENCED_PARAMETER for unused arguments
    UNREFERENCED_PARAMETER(queue);
    UNREFERENCED_PARAMETER(swapchain);
    UNREFERENCED_PARAMETER(source_rect);
    UNREFERENCED_PARAMETER(dest_rect);
    UNREFERENCED_PARAMETER(dirty_rect_count);
    UNREFERENCED_PARAMETER(dirty_rects);
}

static void on_reshade_init_device(reshade::api::device* device)
{
    LogToFile("on_reshade_init_device called.");

    // Start the audio thread only if it's not already running
    if (!g_audio_thread_running.load()) {
        LogToFile("Starting audio capture thread...");
        g_audio_thread_running = true; // Set flag before starting thread
        g_audio_thread = std::thread(AudioCaptureThreadFunc);
    } else {
        LogToFile("Audio thread already running.");
    }
    UNREFERENCED_PARAMETER(device);
}

static void on_reshade_destroy_device(reshade::api::device* device)
{
    LogToFile("on_reshade_destroy_device called.");

    // Signal the audio thread to stop and wait for it to finish
    if (g_audio_thread_running.load()) {
        LogToFile("Stopping audio capture thread...");
        g_audio_thread_running = false; // Signal thread to stop
        if (g_audio_thread.joinable()) {
            g_audio_thread.join(); // Wait for thread to finish
            LogToFile("Audio capture thread joined successfully.");
        } else {
            LogToFile("Audio capture thread was not joinable (already finished?).");
        }
    } else {
        LogToFile("Audio thread was not running.");
    }
    UNREFERENCED_PARAMETER(device);
}

// --- Audio Capture Thread ---

void AudioCaptureThreadFunc() {
    LogToFile("Audio capture thread started.");
    HRESULT hr;

    // 1. Initialize COM for this thread
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        LogToFile("Audio Thread: Failed to initialize COM: " + std::to_string(hr));
        g_audio_thread_running = false; // Ensure flag is false on error exit
        return;
    }
    LogToFile("Audio Thread: COM initialized.");

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCaptureClient = nullptr;
    WAVEFORMATEX* pwfx = nullptr;
    HANDLE hAudioEvent = nullptr; // Event handle for event-driven capture
    UINT32 bufferFrameCount = 0; // Size of the endpoint buffer

    // 2. Get Device Enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        LogToFile("Audio Thread: Failed to create MMDeviceEnumerator: " + std::to_string(hr));
        goto Exit;
    }
    LogToFile("Audio Thread: MMDeviceEnumerator created.");

    // 3. Get Default Render Endpoint (for loopback)
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        LogToFile("Audio Thread: Failed to get default audio endpoint: " + std::to_string(hr));
        goto Exit;
    }
    LogToFile("Audio Thread: Default audio endpoint obtained.");

    // 4. Activate Audio Client
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    if (FAILED(hr)) {
        LogToFile("Audio Thread: Failed to activate IAudioClient: " + std::to_string(hr));
        goto Exit;
    }
    LogToFile("Audio Thread: IAudioClient activated.");

    // 5. Get Mix Format
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
        LogToFile("Audio Thread: Failed to get mix format: " + std::to_string(hr));
        goto Exit;
    }
    LogToFile("Audio Thread: Mix format obtained: SampleRate=" + std::to_string(pwfx->nSamplesPerSec) +
              ", Channels=" + std::to_string(pwfx->nChannels) +
              ", BitsPerSample=" + std::to_string(pwfx->wBitsPerSample) +
              ", FormatTag=" + std::to_string(pwfx->wFormatTag));

    // Check if format is float (common for loopback, easier for analysis)
    bool isFloatFormat = false;
    if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        isFloatFormat = true;
        LogToFile("Audio Thread: Format is IEEE Float.");
    } else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE* pwfex = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
        if (pwfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            isFloatFormat = true;
            LogToFile("Audio Thread: Format is Extensible IEEE Float.");
        }
    }

    if (!isFloatFormat) {
        LogToFile("Audio Thread: Warning - Mix format is not IEEE Float. Analysis requires float data. Addon may not function correctly.");
    }

    // 6. Initialize Audio Client for Loopback (Event-Driven)
    hAudioEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (hAudioEvent == nullptr) {
        LogToFile("Audio Thread: Failed to create audio event handle.");
        goto Exit;
    }

    REFERENCE_TIME hnsRequestedDuration = 200000; // 20ms in 100-nanosecond units
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                  AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                  hnsRequestedDuration,
                                  0,
                                  pwfx,
                                  nullptr);

    if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
        LogToFile("Audio Thread: Initialize failed (AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED). Getting alignment.");
        hr = pAudioClient->GetBufferSize(&bufferFrameCount);
        if (FAILED(hr)) {
             LogToFile("Audio Thread: Failed to get buffer size for alignment: " + std::to_string(hr));
             goto Exit;
        }
        LogToFile("Audio Thread: Buffer frame count for alignment: " + std::to_string(bufferFrameCount));
        hnsRequestedDuration = (REFERENCE_TIME)((10000.0 * 1000 / pwfx->nSamplesPerSec * bufferFrameCount) + 0.5);
        LogToFile("Audio Thread: Retrying Initialize with aligned duration: " + std::to_string(hnsRequestedDuration));
        hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                  AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                  hnsRequestedDuration,
                                  0,
                                  pwfx,
                                  nullptr);
    }

    if (FAILED(hr)) {
        LogToFile("Audio Thread: Failed to initialize IAudioClient: " + std::to_string(hr));
        goto Exit;
    }
    LogToFile("Audio Thread: IAudioClient initialized for loopback.");

    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    if (FAILED(hr)) {
        LogToFile("Audio Thread: Failed to get buffer size: " + std::to_string(hr));
        goto Exit;
    }
    LogToFile("Audio Thread: Actual buffer frame count: " + std::to_string(bufferFrameCount));

    hr = pAudioClient->SetEventHandle(hAudioEvent);
    if (FAILED(hr)) {
        LogToFile("Audio Thread: Failed to set event handle: " + std::to_string(hr));
        goto Exit;
    }

    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
    if (FAILED(hr)) {
        LogToFile("Audio Thread: Failed to get IAudioCaptureClient service: " + std::to_string(hr));
        goto Exit;
    }
    LogToFile("Audio Thread: IAudioCaptureClient obtained.");

    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        LogToFile("Audio Thread: Failed to start audio client: " + std::to_string(hr));
        goto Exit;
    }
    LogToFile("Audio Thread: Audio capture started.");

    while (g_audio_thread_running.load()) {
        DWORD waitResult = WaitForSingleObject(hAudioEvent, 200);

        if (!g_audio_thread_running.load()) break;

        if (waitResult == WAIT_OBJECT_0) {
            BYTE* pData = nullptr;
            UINT32 numFramesAvailable = 0;
            DWORD flags = 0;
            UINT64 devicePosition = 0;
            UINT64 qpcPosition = 0;

            hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, &devicePosition, &qpcPosition);

            if (SUCCEEDED(hr)) {
                if (numFramesAvailable == 0 && !(flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)) {
                    continue;
                }

                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                } else if (pData != nullptr && numFramesAvailable > 0) {
                }

                if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
                    LogToFile("Audio Thread: Data discontinuity reported.");
                }
                 if (flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) {
                    LogToFile("Audio Thread: Timestamp error reported.");
                }

                hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
                if (FAILED(hr)) {
                    LogToFile("Audio Thread: Failed to release buffer: " + std::to_string(hr));
                }
            } else if (hr == AUDCLNT_S_BUFFER_EMPTY) {
                continue;
            } else if (hr == AUDCLNT_E_DEVICE_INVALIDATED || hr == AUDCLNT_E_SERVICE_NOT_RUNNING || hr == AUDCLNT_E_ENDPOINT_CREATE_FAILED) {
                LogToFile("Audio Thread: Audio device invalidated, service stopped or endpoint failed: " + std::to_string(hr) + ". Stopping capture.");
                g_audio_thread_running = false;
                break;
            } else {
                LogToFile("Audio Thread: Error getting buffer: " + std::to_string(hr));
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        } else if (waitResult == WAIT_TIMEOUT) {
            continue;
        } else {
            LogToFile("Audio Thread: WaitForSingleObject failed: " + std::to_string(GetLastError()));
            g_audio_thread_running = false;
            break;
        }
    }

    LogToFile("Audio capture loop finished.");

Exit:
    LogToFile("Audio Thread: Cleaning up audio resources...");
    if (pAudioClient) {
        pAudioClient->Stop();
        LogToFile("Audio Thread: Audio client stopped.");
    }
    if (hAudioEvent) {
        CloseHandle(hAudioEvent);
        hAudioEvent = nullptr;
        LogToFile("Audio Thread: Event handle closed.");
    }
    if (pCaptureClient) {
        pCaptureClient->Release();
        pCaptureClient = nullptr;
        LogToFile("Audio Thread: Capture client released.");
    }
    if (pAudioClient) {
        pAudioClient->Release();
        pAudioClient = nullptr;
        LogToFile("Audio Thread: Audio client released.");
    }
    if (pDevice) {
        pDevice->Release();
        pDevice = nullptr;
        LogToFile("Audio Thread: Device released.");
    }
    if (pEnumerator) {
        pEnumerator->Release();
        pEnumerator = nullptr;
        LogToFile("Audio Thread: Enumerator released.");
    }
    if (pwfx) {
        CoTaskMemFree(pwfx);
        pwfx = nullptr;
        LogToFile("Audio Thread: Wave format memory freed.");
    }

    CoUninitialize();
    LogToFile("Audio Thread: COM uninitialized.");
    LogToFile("Audio capture thread finished execution.");
    g_audio_thread_running = false;
}


// ---------------------------------------------
// Process-Specific Audio Capture Module
// Captures audio from only the current process using WASAPI Session Management
// ---------------------------------------------
#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <windows.h>
#include "audio_analysis.h"

/**
 * @brief Configuration for process-specific audio capture
 */
struct ProcessAudioCaptureConfig {
    bool use_process_audio = false;     // Enable process-specific capture instead of system-wide
    DWORD target_process_id = 0;        // Target process ID (0 = current process)
    float fallback_timeout = 5.0f;     // Seconds to wait before falling back to system audio
    
    ProcessAudioCaptureConfig() : use_process_audio(false), target_process_id(0), fallback_timeout(5.0f) {}
};

/**
 * @brief Starts the process-specific audio capture thread.
 * @param config Analysis configuration (FFT size, bands).
 * @param process_config Process-specific capture configuration.
 * @param running Atomic flag to control thread lifetime.
 * @param thread Thread object (will be started).
 * @param data_mutex Mutex protecting the analysis data.
 * @param data Analysis data to be updated by the thread.
 */
void StartProcessAudioCaptureThread(const AudioAnalysisConfig& config, 
                                   const ProcessAudioCaptureConfig& process_config,
                                   std::atomic_bool& running, 
                                   std::thread& thread, 
                                   std::mutex& data_mutex, 
                                   AudioAnalysisData& data);

/**
 * @brief Stops the process-specific audio capture thread.
 * @param running Atomic flag to signal thread to stop.
 * @param thread Thread object (will be joined).
 */
void StopProcessAudioCaptureThread(std::atomic_bool& running, std::thread& thread);

/**
 * @brief Finds an audio session for a specific process ID.
 * @param device_enumerator The MMDeviceEnumerator instance.
 * @param target_pid The process ID to find (0 = current process).
 * @param out_session_control Output parameter for the session control interface.
 * @param out_audio_client Output parameter for the audio client interface.
 * @return S_OK if successful, error code otherwise.
 */
HRESULT FindProcessAudioSession(IMMDeviceEnumerator* device_enumerator,
                               DWORD target_pid,
                               IAudioSessionControl** out_session_control,
                               IAudioClient** out_audio_client);

/**
 * @brief Checks if process-specific audio capture is available for the current process.
 * @return True if process-specific capture is available, false otherwise.
 */
bool IsProcessAudioCaptureAvailable();

/**
 * @brief Gets information about the current process's audio session.
 * @param session_name Output buffer for session display name.
 * @param session_name_size Size of the session_name buffer.
 * @param process_id Output parameter for the process ID.
 * @return True if successful, false otherwise.
 */
bool GetCurrentProcessAudioInfo(wchar_t* session_name, size_t session_name_size, DWORD* process_id);

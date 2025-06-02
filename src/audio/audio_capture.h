// ---------------------------------------------
// Audio Capture Module
// Manages audio capture using different providers (System/Process)
// ---------------------------------------------
#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include "audio_analysis.h"
#include "audio_capture_manager.h"

// Global audio capture manager
extern std::unique_ptr<AudioCaptureManager> g_audio_capture_manager;

/**
 * @brief Initializes the audio capture system
 * @return true if initialization succeeded
 */
bool InitializeAudioCapture();

/**
 * @brief Uninitializes the audio capture system
 */
void UninitializeAudioCapture();

/**
 * @brief Starts the audio capture thread using the current provider.
 * @param config Analysis configuration (FFT size, bands).
 * @param running Atomic flag to control thread lifetime.
 * @param thread Thread object (will be started).
 * @param data_mutex Mutex protecting the analysis data.
 * @param data Analysis data to be updated by the thread.
 */
void StartAudioCaptureThread(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);

/**
 * @brief Stops the audio capture thread.
 * @param running Atomic flag to signal thread to stop.
 * @param thread Thread object (will be joined).
 */
void StopAudioCaptureThread(std::atomic_bool& running, std::thread& thread);

/**
 * @brief Initializes audio device notifications.
 */
inline void InitAudioDeviceNotification() {}

/**
 * @brief Uninitializes audio device notifications.
 */
inline void UninitAudioDeviceNotification() {}

/**
 * @brief Checks and restarts the audio capture thread if necessary.
 * @param config Analysis configuration (FFT size, bands).
 * @param running Atomic flag to control thread lifetime.
 * @param thread Thread object (will be restarted if needed).
 * @param data_mutex Mutex protecting the analysis data.
 * @param data Analysis data to be updated by the thread.
 */
void CheckAndRestartAudioCapture(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);

/**
 * @brief Sets the preferred audio capture provider
 * @param providerType The provider type to use (0 = System, 1 = Process)
 * @return true if the provider was set successfully
 */
bool SetAudioCaptureProvider(int providerType);

/**
 * @brief Gets the current audio capture provider type
 * @return Current provider type (0 = System, 1 = Process)
 */
int GetAudioCaptureProvider();

/**
 * @brief Gets available audio capture providers
 * @return Vector of available provider types
 */
std::vector<int> GetAvailableAudioCaptureProviders();

/**
 * @brief Gets the name of an audio capture provider
 * @param providerType Provider type
 * @return Human-readable provider name
 */
std::string GetAudioCaptureProviderName(int providerType);

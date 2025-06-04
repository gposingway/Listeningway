#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include "audio_analysis.h"
#include "audio_capture_manager.h"

extern std::unique_ptr<AudioCaptureManager> g_audio_capture_manager;

// Audio capture system management
bool InitializeAudioCapture();
void UninitializeAudioCapture();

// Audio capture thread management
void StartAudioCaptureThread(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);
void StopAudioCaptureThread(std::atomic_bool& running, std::thread& thread);

// Audio device notifications (inline no-ops)
inline void InitAudioDeviceNotification() {}
inline void UninitAudioDeviceNotification() {}

// Audio capture restart and provider management
void CheckAndRestartAudioCapture(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);
bool SetAudioCaptureProvider(int providerType);
int GetAudioCaptureProvider();

/**
 * @brief Gets available audio capture providers
 * @return Vector of available provider infos
 */
std::vector<AudioProviderInfo> GetAvailableAudioCaptureProviders();

/**
 * @brief Gets the name of an audio capture provider by code
 * @param providerCode Provider code string
 * @return Human-readable provider name
 */
std::string GetAudioCaptureProviderName(const std::string& providerCode);

// Overlay API: Switch provider and restart capture thread if running
bool SwitchAudioCaptureProviderAndRestart(int providerType, const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);

// Overlay API: Switch provider by code and restart capture thread if running
bool SwitchAudioCaptureProviderByCodeAndRestart(const std::string& providerCode, const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);

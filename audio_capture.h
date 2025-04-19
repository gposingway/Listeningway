// ---------------------------------------------
// Audio Capture Module
// Captures audio from the system using WASAPI loopback
// ---------------------------------------------
#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "audio_analysis.h"

/**
 * @brief Starts the audio capture thread.
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
void InitAudioDeviceNotification();

/**
 * @brief Uninitializes audio device notifications.
 */
void UninitAudioDeviceNotification();

/**
 * @brief Checks and restarts the audio capture thread if necessary.
 * @param config Analysis configuration (FFT size, bands).
 * @param running Atomic flag to control thread lifetime.
 * @param thread Thread object (will be restarted if needed).
 * @param data_mutex Mutex protecting the analysis data.
 * @param data Analysis data to be updated by the thread.
 */
void CheckAndRestartAudioCapture(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);

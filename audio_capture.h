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

// Starts the audio capture thread.
//   - config: analysis configuration (FFT size, bands)
//   - running: atomic flag to control thread lifetime
//   - thread: thread object (will be started)
//   - data_mutex: mutex protecting the analysis data
//   - data: analysis data to be updated by the thread
void StartAudioCaptureThread(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);

// Stops the audio capture thread.
//   - running: atomic flag to signal thread to stop
//   - thread: thread object (will be joined)
void StopAudioCaptureThread(std::atomic_bool& running, std::thread& thread);

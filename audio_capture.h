#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "audio_analysis.h"

void StartAudioCaptureThread(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);
void StopAudioCaptureThread(std::atomic_bool& running, std::thread& thread);

#pragma once
#include "audio_analysis.h"
#include "audio_capture_manager.h"
#include <atomic>
#include <thread>
#include <mutex>

namespace Listeningway {

class AudioSystemManager {
public:
    AudioSystemManager(AudioAnalyzer& analyzer,
                       std::atomic_bool& audioThreadRunning,
                       std::thread& audioThread,
                       std::mutex& audioDataMutex,
                       AudioAnalysisData& audioData,
                       AudioCaptureManager* captureManager = nullptr)
        : analyzer_(analyzer),
          audio_thread_running_(audioThreadRunning),
          audio_thread_(audioThread),
          audio_data_mutex_(audioDataMutex),
          audio_data_(audioData),
          capture_manager_(captureManager) {}

    AudioAnalyzer& Analyzer() { return analyzer_; }
    std::atomic_bool& AudioThreadRunning() { return audio_thread_running_; }
    std::thread& AudioThread() { return audio_thread_; }
    std::mutex& AudioDataMutex() { return audio_data_mutex_; }
    AudioAnalysisData& AudioData() { return audio_data_; }
    AudioCaptureManager* GetAudioCaptureManager() { return capture_manager_; }
    void SetAudioCaptureManager(AudioCaptureManager* mgr) { capture_manager_ = mgr; }

private:
    AudioAnalyzer& analyzer_;
    std::atomic_bool& audio_thread_running_;
    std::thread& audio_thread_;
    std::mutex& audio_data_mutex_;
    AudioAnalysisData& audio_data_;
    AudioCaptureManager* capture_manager_;
};

} // namespace Listeningway

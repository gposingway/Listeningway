// ---------------------------------------------
// Overlay Module
// Provides a debug ImGui overlay for real-time audio analysis data
// ---------------------------------------------
#pragma once
#include <vector>
#include <mutex>
#include "audio_analysis.h"

// Draws the Listeningway debug overlay using ImGui.
//   - data: current audio analysis data (volume, bands, beat)
//   - data_mutex: mutex protecting the analysis data
//   - capture_active: whether audio capture is running
void DrawListeningwayDebugOverlay(const AudioAnalysisData& data, std::mutex& data_mutex, bool capture_active);

// ---------------------------------------------
// Overlay Module
// Provides a debug ImGui overlay for real-time audio analysis data
// ---------------------------------------------
#pragma once
#include <vector>
#include <mutex>
#include "audio_analysis.h"

/**
 * @brief Draws the Listeningway debug overlay using ImGui.
 * Shows volume, beat, and frequency bands in real time.
 * @param data Current audio analysis data (volume, bands, beat).
 * @param data_mutex Mutex protecting the analysis data.
 */
void DrawListeningwayDebugOverlay(const AudioAnalysisData& data, std::mutex& data_mutex);
